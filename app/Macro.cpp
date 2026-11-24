// Macro.cpp
// Implementation of the macro expander (see Macro.h).

#include "Macro.h"
#include "Tokenizer.h"
#include "Format.h"

// Removes comments and unnecessary spaces from an assembly line.
// Everything after ';' is considered a comment.
static std::string codeOf(const std::string& text) {
    std::string code = text;
    int semicolon = (int)code.find(';');
    if (semicolon != (int)std::string::npos) {
        code = code.substr(0, semicolon);
    }
    return trim(code);
}

// Checks whether a line starts with a specific directive, such as
// ".MACRO" or ".ENDM".
static bool startsWithWord(const std::string& code, const std::string& word) {
    if ((int)code.length() < (int)word.length()) {
        return false;
    }
    if (toUpper(code.substr(0, word.length())) != word) {
        return false;
    }
    // the directive must be a whole word: ".MACROX" is something else
    return code.length() == word.length() || code[word.length()] == ' ' || code[word.length()] == '\t';
}

// Replaces the formal parameters in a macro body line with the actual
// arguments provided in the macro call.
// It also replaces "@@" with the unique ID assigned to that macro call.
static std::string substitute(const std::string& text, const MacroFrame& frame) {
    std::string result;
    int i = 0;
    int n = (int)text.length();
    while (i < n) {
        // If we find "@@", replace it with this macro call's unique ID.
        if (text[i] == '@' && i + 1 < n && text[i + 1] == '@') {
            result += numberToString(frame.id);
            i += 2;
        } else if (isLetterOrUnderscore(text[i])) {
            int start = i;                              // a name: read all of it
            while (i < n && isIdentifierChar(text[i])) {
                i++;
            }
            std::string word = text.substr(start, i - start);
            bool replaced = false;
            for (int p = 0; p < frame.def.params.size(); p++) {
                if (frame.def.params[p] == word) {
                    result += frame.args[p];
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                result += word;
            }
        } else if (isDigitChar(text[i])) {
            int start = i;                              // a number like 0x1F: copy it untouched
            while (i < n && isIdentifierChar(text[i])) {
                i++;
            }
            result += text.substr(start, i - start);
        } else {
            result.push_back(text[i]);
            i++;
        }
    }
    return result;
}

// ---------------------------------------------------------------
// Step 1: find the definitions (.MACRO ... .ENDM) and store them.
// Every other line goes to "rest" together with its line number.
// ---------------------------------------------------------------
void MacroExpander::collectDefinitions(const DynamicArray<std::string>& rawLines,
                                       DynamicArray<ExpandedLine>& rest,
                                       DynamicArray<MacroError>& errors) {
    bool inside = false;            // are we between .MACRO and .ENDM?
    bool valid = false;             // is the macro we are reading usable?
    MacroDef current;
    int startLine = 0;

    for (int i = 0; i < rawLines.size(); i++) {
        int lineNumber = i + 1;
        std::string code = codeOf(rawLines[i]);

        if (startsWithWord(code, ".MACRO")) {
            if (inside) {
                MacroError e;
                e.line = lineNumber;
                e.message = "macro definitions cannot be nested";
                errors.pushBack(e);
                continue;
            }
            inside = true;
            valid = true;
            startLine = lineNumber;
            current = MacroDef();

            // Extract everything after ".MACRO".
            // Example: ".MACRO swap, a, b"
            // We need to separate the name and parameters using commas.
            std::string rest1 = trim(code.substr(6));
            DynamicArray<std::string> parts;
            std::string piece;
            for (int k = 0; k < (int)rest1.length(); k++) {
                if (rest1[k] == ',') {
                    parts.pushBack(trim(piece));
                    piece = std::string();
                } else {
                    piece.push_back(rest1[k]);
                }
            }
            if (!trim(piece).empty() || parts.size() > 0) {
                parts.pushBack(trim(piece));
            }

            MacroError e;
            e.line = lineNumber;
            if (parts.size() == 0 || parts[0].empty()) {
                e.message = "macro definition needs a name";
                errors.pushBack(e);
                valid = false;
            } else if (!isIdentifier(parts[0])) {
                e.message = "invalid macro name '" + parts[0] + "'";
                errors.pushBack(e);
                valid = false;
            } else {
                current.name = toUpper(parts[0]);
                if (opcodes.contains(current.name)) {
                    e.message = "macro name '" + current.name + "' is already an instruction";
                    errors.pushBack(e);
                    valid = false;
                } else if (macros.contains(current.name)) {
                    e.message = "macro '" + current.name + "' is already defined";
                    errors.pushBack(e);
                    valid = false;
                }
            }
            for (int p = 1; p < parts.size() && valid; p++) {
                bool duplicate = false;
                for (int q = 0; q < current.params.size(); q++) {
                    if (current.params[q] == parts[p]) {
                        duplicate = true;
                    }
                }
                if (!isIdentifier(parts[p])) {
                    e.message = "invalid macro parameter '" + parts[p] + "'";
                    errors.pushBack(e);
                    valid = false;
                } else if (duplicate) {
                    e.message = "duplicate macro parameter '" + parts[p] + "'";
                    errors.pushBack(e);
                    valid = false;
                } else {
                    current.params.pushBack(parts[p]);
                }
            }
        } else if (startsWithWord(code, ".ENDM")) {
            if (!inside) {
                MacroError e;
                e.line = lineNumber;
                e.message = ".ENDM without .MACRO";
                errors.pushBack(e);
            } else {
                if (valid) {
                    macros.put(current.name, current);
                }
                inside = false;
            }
        } else if (inside) {
            current.body.pushBack(rawLines[i]);       // a line of the body
        } else {
            ExpandedLine line;
            line.text = rawLines[i];
            line.lineNumber = lineNumber;
            rest.pushBack(line);
        }
    }

    if (inside) {
        MacroError e;
        e.line = startLine;
        e.message = "macro '" + current.name + "' has no .ENDM";
        errors.pushBack(e);
    }
}

// ---------------------------------------------------------------
// Step 2: expand the calls.
// ---------------------------------------------------------------

// Look at one line. If it calls a macro, push a frame for it on the stack of
// active macros. Otherwise copy the line to the output.
void MacroExpander::handleLine(const std::string& text, int lineNumber,
                               DynamicArray<ExpandedLine>& out, DynamicArray<MacroError>& errors,
                               Stack<MacroFrame>& active) {
    SourceLine parsed;
    tokenizeLine(text, lineNumber, parsed);

    MacroDef def;
    bool isCall = parsed.error.empty() && !parsed.mnemonic.empty() &&
                  macros.get(parsed.mnemonic, def);
    if (!isCall) {
        ExpandedLine line;
        line.text = text;
        line.lineNumber = lineNumber;
        out.pushBack(line);
        return;
    }

    // a label in front of the call stays: it points to the first expanded line
    if (!parsed.label.empty()) {
        ExpandedLine labelLine;
        labelLine.text = parsed.label + ":";
        labelLine.lineNumber = lineNumber;
        out.pushBack(labelLine);
    }

    if (parsed.operands.size() != def.params.size()) {
        MacroError e;
        e.line = lineNumber;
        e.message = "wrong number of arguments for macro " + def.name + " (expected " +
                    numberToString(def.params.size()) + ", got " +
                    numberToString(parsed.operands.size()) + ")";
        errors.pushBack(e);
        return;
    }

    MacroFrame frame;
    frame.def = def;
    frame.args = parsed.operands;
    frame.nextLine = 0;
    expansionCounter++;
    frame.id = expansionCounter;
    frame.callLine = lineNumber;
    if (!active.push(frame)) {                     // the stack is full
        MacroError e;
        e.line = lineNumber;
        e.message = "macros are nested too deeply (limit " + numberToString(MAX_MACRO_DEPTH) +
                    "), does macro " + def.name + " call itself?";
        errors.pushBack(e);
    }
}

// Work through the stack of active macros until it is empty.
// The macro on TOP is always the one that is expanded next.
void MacroExpander::runActiveMacros(DynamicArray<ExpandedLine>& out, DynamicArray<MacroError>& errors,
                                    Stack<MacroFrame>& active) {
    while (!active.isEmpty()) {
        MacroFrame frame;
        active.pop(frame);                          // take the top frame

        if (frame.nextLine >= frame.def.body.size()) {
            continue;                               // finished: leave it popped
        }
        std::string line = substitute(frame.def.body[frame.nextLine], frame);
        frame.nextLine++;
        active.push(frame);                         // put it back (the limit cannot be hit: it was there before)

        // errors inside a body are reported at the line of the original call
        handleLine(line, frame.callLine, out, errors, active);
    }
}

void MacroExpander::expand(const DynamicArray<std::string>& rawLines,
                           DynamicArray<ExpandedLine>& out, DynamicArray<MacroError>& errors) {
    macros.clear();
    expansionCounter = 0;

    DynamicArray<ExpandedLine> rest;
    collectDefinitions(rawLines, rest, errors);

    Stack<MacroFrame> active(MAX_MACRO_DEPTH);
    for (int i = 0; i < rest.size(); i++) {
        handleLine(rest[i].text, rest[i].lineNumber, out, errors, active);
        runActiveMacros(out, errors, active);
    }
}
