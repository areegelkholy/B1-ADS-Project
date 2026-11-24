// Macro.h
// ---------------------------------------------------------------
// Challenge: MACRO EXPANSION.
//
// A macro is a named piece of source text with parameters. Before the
// assembler starts (before pass 1) we replace every macro call by the
// macro's body, so pass 1 and pass 2 never see a macro at all.
//
//     .MACRO swap, a, b          ; definition: name, then the parameters
//         MOV R7, a
//         MOV a, b
//         MOV b, R7
//     .ENDM
//
//             swap R1, R2        ; call: becomes the three MOV lines
//
//   - Parameters are replaced as text: every "a" in the body becomes "R1".
//   - "@@" in the body becomes a number that is different for every call.
//     Use it to make labels unique:  skip@@:  ->  skip1:  skip2: ...
//   - A macro body may call other macros (nesting).
//
// DATA STRUCTURES:
//   HashMap<MacroDef>   macro name -> definition        O(1) lookup
//   Stack<MacroFrame>   the STACK OF ACTIVE MACRO CONTEXTS: one frame for
//                       every macro that is being expanded right now.
//                       A call inside a body pushes a new frame on top;
//                       when its body is finished the frame is popped and we
//                       go on with the macro below it.
//                       The stack has a size limit, so a macro that calls
//                       itself is stopped with an error (no endless loop).
//
// Error messages use the line number of the line that CALLS the macro,
// so the programmer finds the place in his own file.
// ---------------------------------------------------------------

#ifndef MACRO_H
#define MACRO_H

#include <string>
#include "../ds/DynamicArray.h"
#include "../ds/HashMap.h"
#include "../ds/Stack.h"
#include "Isa.h"
// Maximum number of macros that can be active/nested at the same time.
const int MAX_MACRO_DEPTH = 16;

// Represents one line of the program after macro expansion.
// Stores the expanded text and its original source line number.
struct ExpandedLine {
    std::string text; // The actual assembly instruction or line.
    int lineNumber; // Original line number, used for error reporting.
    ExpandedLine() : lineNumber(0) {}
};
// Stores an error encountered during macro definition or expansion.
struct MacroError {
    int line;
    std::string message;
    MacroError() : line(0) {}
};

// Stores the definition of one macro.struct MacroDef {
    std::string name;                       // upper case
    DynamicArray<std::string> params;     // Stores the formal parameters declared in the .MACRO definition.
    DynamicArray<std::string> body;         // the source lines between .MACRO and .ENDM
};

// Represents one macro that is currently being expanded.
// Each active macro has its own frame in the active macro Stack.struct MacroFrame {
    MacroDef def;
    DynamicArray<std::string> args;
    int nextLine;                           // index of the next body line to expand
    int id;                                 // number that replaces "@@"
    int callLine;                           // line of the original call (for errors)
    MacroFrame() : nextLine(0), id(0), callLine(0) {}
};

class MacroExpander {
private:
    // Reference to the opcode HashMap.
        // Used to prevent defining a macro with the same name as an instruction.
    const HashMap<OpInfo>& opcodes;
    HashMap<MacroDef> macros;
    int expansionCounter;                   // gives every call its own "@@" number

    void collectDefinitions(const DynamicArray<std::string>& rawLines,
                            DynamicArray<ExpandedLine>& rest,
                            DynamicArray<MacroError>& errors);
    // Checks one assembly line to determine whether it is a macro call.
    // If it is a macro, creates a frame and pushes it onto the active Stack.
    // Otherwise, copies the line directly to the expanded output.

    void handleLine(const std::string& text, int lineNumber,
                    DynamicArray<ExpandedLine>& out, DynamicArray<MacroError>& errors,
                    Stack<MacroFrame>& active);
    // Processes the active macro Stack and expands macro bodies.
       // The top frame is expanded first, allowing nested macros to be handled.
    void runActiveMacros(DynamicArray<ExpandedLine>& out, DynamicArray<MacroError>& errors,
                         Stack<MacroFrame>& active);

public:
    explicit MacroExpander(const HashMap<OpInfo>& opcodeTable)
        : opcodes(opcodeTable), expansionCounter(0) {}

    // rawLines: the file, one string per line (line 1 = index 0).
    // out: the program without macros. Lines of the definitions are removed.
    void expand(const DynamicArray<std::string>& rawLines,
                DynamicArray<ExpandedLine>& out, DynamicArray<MacroError>& errors);

    int macroCount() const { return macros.size(); }
};

#endif
