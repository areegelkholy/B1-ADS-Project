// Implementation of the two-pass assembler.

#include "Assembler.h"
#include "Expression.h"
#include "Format.h"
#include "Macro.h"
#include "../ds/Sort.h"

Assembler::Assembler() : errorOrder(0), wordsWritten(0) {
    buildOpcodeTable(opcodes);
}


// Errors


void Assembler::addError(int lineNumber, const std::string& message) {
    AsmError error;
    error.line = lineNumber;
    error.order = errorOrder;
    error.message = message;
    errorOrder++;
    errors.pushBack(error);
}


// Helpers


// Check that a text is a register R0 to R7, else report an error.
bool Assembler::readRegister(const std::string& text, int lineNumber, int& reg) {
    if (parseRegister(text, reg)) {
        return true;
    }
    addError(lineNumber, "invalid register '" + text + "' (use R0 to R7)");
    return false;
}

// Evaluate an expression and check that it fits the field 
bool Assembler::readValue(const std::string& text, int lineNumber, int minValue, int maxValue,
                          const char* what, int& value) {
    long long result = 0;
    std::string message;
    if (!evaluateExpression(text, symbols, result, message)) {
        addError(lineNumber, message);
        return false;
    }
    if (result < minValue || result > maxValue) {
        addError(lineNumber, std::string(what) + " " + numberToString(result) +
                 " is out of range (" + numberToString(minValue) + " to " +
                 numberToString(maxValue) + ")");
        return false;
    }
    value = (int)result;
    return true;
}

// Split "K+1(R3)" into the offset text "K+1" and the register text "R3".
bool Assembler::splitMemoryOperand(const std::string& text, std::string& offsetText, std::string& regText) {
    int last = (int)text.length() - 1;
    if (last < 0 || text[last] != ')') {
        return false;
    }
    int open = (int)text.rfind('(');      // the LAST "(" belongs to the register
    if (open == (int)std::string::npos) {
        return false;
    }
    offsetText = text.substr(0, open);
    regText = text.substr(open + 1, last - open - 1);
    return true;
}


// The main function


bool Assembler::assemble(const std::string& sourceText) {
    // start from a clean state (so one Assembler can be used again)
    symbols.clear();
    lines.clear();
    listing.clear();
    errors.clear();
    errorOrder = 0;
    wordsWritten = 0;
    image.fill(MEMORY_SIZE, 0);
    written.fill(MEMORY_SIZE, 0);

    // cut the text into lines and tokenize each one
    DynamicArray<std::string> rawLines;
    splitLines(sourceText, rawLines);
    //challenge
    // expand the macros first: afterwards there are no macros left
    DynamicArray<ExpandedLine> expanded;
    DynamicArray<MacroError> macroErrors;
    MacroExpander expander(opcodes);
    expander.expand(rawLines, expanded, macroErrors);
    for (int i = 0; i < macroErrors.size(); i++) {
        addError(macroErrors[i].line, macroErrors[i].message);
    }

    for (int i = 0; i < expanded.size(); i++) {
        SourceLine line;
        tokenizeLine(expanded[i].text, expanded[i].lineNumber, line);
        lines.pushBack(line);
    }

    pass1();
    pass2();

    // report the errors in the order of the source file
    heapSort(errors);
    return errors.isEmpty();
}


// 1- addresses and the symbol table


// Store a new symbol. Labels and .EQU names share one namespace, so a second definition of the same name is an error.
void Assembler::defineSymbol(const std::string& name, long long value, int lineNumber) {
    if (symbols.contains(name)) {
        addError(lineNumber, "duplicate symbol '" + name + "'");
        return;
    }
    if (value < -2147483647LL || value > 2147483647LL) {
        addError(lineNumber, "value of '" + name + "' is too large");
        return;
    }
    symbols.put(name, (int)value);
}

// Directives need an exact number of operands. Reports the error if it is wrong.
bool Assembler::checkOperandCount(SourceLine& line, int expected) {
    if (line.operands.size() != expected) {
        addError(line.lineNumber, "wrong number of operands for " + line.mnemonic +
                 " (expected " + numberToString(expected) + ", got " +
                 numberToString(line.operands.size()) + ")");
        line.bad = true;
        return false;
    }
    return true;
}

void Assembler::pass1Directive(SourceLine& line, int& location) {
    long long value = 0;
    std::string message;

    if (line.mnemonic == ".ORG") {
        if (!checkOperandCount(line, 1)) return;
        if (!evaluateExpression(line.operands[0], symbols, value, message)) {
            addError(line.lineNumber, message);
            line.bad = true;
            return;
        }
        if (value < 0 || value > MAX_ADDRESS) {
            addError(line.lineNumber, "address " + numberToString(value) + " is out of range (0 to 255)");
            line.bad = true;
            return;
        }
        location = (int)value;                    
        line.address = location;

    } else if (line.mnemonic == ".EQU") {
        if (!checkOperandCount(line, 2)) return;
        if (!isIdentifier(line.operands[0])) {
            addError(line.lineNumber, "invalid symbol name '" + line.operands[0] + "'");
            line.bad = true;
            return;
        }
        if (!evaluateExpression(line.operands[1], symbols, value, message)) {
            addError(line.lineNumber, message);
            line.bad = true;
            return;
        }
        defineSymbol(line.operands[0], value, line.lineNumber);

    } else if (line.mnemonic == ".WORD") {
        if (line.operands.size() < 1) {
            addError(line.lineNumber, "wrong number of operands for .WORD (expected at least 1, got 0)");
            line.bad = true;
            return;
        }
        line.size = line.operands.size();        
        location = location + line.size;

    } else if (line.mnemonic == ".SPACE") {
        if (!checkOperandCount(line, 1)) return;
        if (!evaluateExpression(line.operands[0], symbols, value, message)) {
            addError(line.lineNumber, message);
            line.bad = true;
            return;
        }
        if (value < 0 || value > MEMORY_SIZE) {
            addError(line.lineNumber, "size " + numberToString(value) + " is out of range (0 to 256)");
            line.bad = true;
            return;
        }
        line.size = (int)value;
        location = location + line.size;

    } else {
        addError(line.lineNumber, "unknown directive '" + line.mnemonic + "'");
        line.bad = true;
    }
}

void Assembler::pass1() {
    int location = 0;                             
    for (int i = 0; i < lines.size(); i++) {
        SourceLine& line = lines[i];
        line.address = location;

        if (!line.error.empty()) {               
            addError(line.lineNumber, line.error);
            line.bad = true;
            continue;
        }

        if (!line.label.empty()) {
            defineSymbol(line.label, location, line.lineNumber);
        }

        if (line.mnemonic.empty()) {
            continue;                           
        }

        if (line.mnemonic[0] == '.') {
            pass1Directive(line, location);
        } else {
            OpInfo info;
            if (!opcodes.get(line.mnemonic, info)) {
                addError(line.lineNumber, "unknown mnemonic '" + line.mnemonic + "'");
                line.bad = true;
            } else {
                line.size = 1;                    
                location = location + 1;
            }
        }
    }
}


// 2- encoding


/* Put one word into the memory image and into the listing.
 "index" is the position of the word inside its source line (0 = first).*/
bool Assembler::emitWord(const SourceLine& line, int index, int word) {
    int address = line.address + index;
    if (address > MAX_ADDRESS) {
        addError(line.lineNumber, "address 0x" + hexString(address, 2) + " is above 0xFF");
        return false;
    }
    if (written[address] == 1) {
        addError(line.lineNumber, "address 0x" + hexString(address, 2) +
                 " is already used by another word");
        return false;
    }
    image[address] = word & 0xFFFF;
    written[address] = 1;
    wordsWritten++;

    ListingEntry entry;
    entry.address = address;
    entry.word = word & 0xFFFF;
    entry.hasWord = true;
    entry.lineNumber = line.lineNumber;
    if (index == 0) {
        entry.text = line.text;                 
    }
    listing.pushBack(entry);
    return true;
}

// MOV, INC, DEC and CLR are rewritten into one real instruction each.
bool Assembler::encodePseudo(int lineNumber, const OpInfo& info, const DynamicArray<std::string>& ops, int& word) {
    DynamicArray<std::string> newOps;
    std::string realName = "ADD";
    if (info.extra == PSEUDO_MOV) {               // MOV rd, rs  ->  ADD rd, rs, R0
        newOps.pushBack(ops[0]);
        newOps.pushBack(ops[1]);
        newOps.pushBack("R0");
    } else if (info.extra == PSEUDO_INC) {        // INC rd      ->  ADDI rd, rd, 1
        realName = "ADDI";
        newOps.pushBack(ops[0]);
        newOps.pushBack(ops[0]);
        newOps.pushBack("1");
    } else if (info.extra == PSEUDO_DEC) {        // DEC rd      ->  ADDI rd, rd, -1
        realName = "ADDI";
        newOps.pushBack(ops[0]);
        newOps.pushBack(ops[0]);
        newOps.pushBack("-1");
    } else {                                      // CLR rd      ->  ADD rd, R0, R0
        newOps.pushBack(ops[0]);
        newOps.pushBack("R0");
        newOps.pushBack("R0");
    }
    OpInfo real;
    opcodes.get(realName, real);
    return encode(lineNumber, real, newOps, word);
}

/*Encode ONE instruction into its 16-bit word. Returns false if there was an error. 
We check every operand  so that all mistakes of the line are reported at once.*/
bool Assembler::encode(int lineNumber, const OpInfo& info, const DynamicArray<std::string>& ops, int& word) {
    int rd = 0, rs = 0, rt = 0, value = 0;
    bool ok = true;
    int op = info.opcode << 12;                  // the opcode goes into bits 15..12

    switch (info.format) {
        case FMT_R:                              // ADD rd, rs, rt
            if (!readRegister(ops[0], lineNumber, rd)) ok = false;
            if (!readRegister(ops[1], lineNumber, rs)) ok = false;
            if (!readRegister(ops[2], lineNumber, rt)) ok = false;
            word = op | (rd << 9) | (rs << 6) | (rt << 3);
            break;

        case FMT_ADDI:                           // ADDI rd, rs, imm6
            if (!readRegister(ops[0], lineNumber, rd)) ok = false;
            if (!readRegister(ops[1], lineNumber, rs)) ok = false;
            if (!readValue(ops[2], lineNumber, IMM6_MIN, IMM6_MAX, "immediate", value)) ok = false;
            word = op | (rd << 9) | (rs << 6) | (value & 0x3F);
            break;

        case FMT_LDI:                            // LDI rd, imm9
            if (!readRegister(ops[0], lineNumber, rd)) ok = false;
            if (!readValue(ops[1], lineNumber, IMM9_MIN, IMM9_MAX, "immediate", value)) ok = false;
            word = op | (rd << 9) | (value & 0x1FF);
            break;

        case FMT_MEM: {                          // LW / SW rd, off(rs)
            if (!readRegister(ops[0], lineNumber, rd)) ok = false;
            std::string offsetText;
            std::string regText;
            if (!splitMemoryOperand(ops[1], offsetText, regText)) {
                addError(lineNumber, "expected offset(register), for example 0(R3), but found '" + ops[1] + "'");
                ok = false;
            } else {
                if (!readRegister(regText, lineNumber, rs)) ok = false;
                if (offsetText.empty()) {
                    value = 0;                   // "(R3)" means offset 0
                } else if (!readValue(offsetText, lineNumber, IMM6_MIN, IMM6_MAX, "offset", value)) {
                    ok = false;
                }
            }
            word = op | (rd << 9) | (rs << 6) | (value & 0x3F);
            break;
        }

        case FMT_CMP:                            // CMP rs, rt
            if (!readRegister(ops[0], lineNumber, rs)) ok = false;
            if (!readRegister(ops[1], lineNumber, rt)) ok = false;
            word = op | (rs << 6) | (rt << 3);
            break;

        case FMT_BRANCH:                         // JMP/BEQ... label: condition in bits 11..9
            if (!readValue(ops[0], lineNumber, 0, MAX_ADDRESS, "branch target", value)) ok = false;
            word = op | (info.extra << 9) | value;
            break;

        case FMT_CALL:                           // CALL label
            if (!readValue(ops[0], lineNumber, 0, MAX_ADDRESS, "call target", value)) ok = false;
            word = op | value;
            break;

        case FMT_RET:                            
            word = op;
            break;

        case FMT_PUSH:                         
            if (!readRegister(ops[0], lineNumber, rs)) ok = false;
            word = op | (rs << 6);
            break;

        case FMT_POP:                            
            if (!readRegister(ops[0], lineNumber, rd)) ok = false;
            word = op | (rd << 9);
            break;

        case FMT_SYS:                            
            word = op | info.extra;
            break;

        case FMT_OUT:                            
            if (!readRegister(ops[0], lineNumber, rs)) ok = false;
            word = op | (rs << 6) | info.extra;
            break;

        default:
            ok = false;
            break;
    }
    return ok;
}

void Assembler::pass2() {
    for (int i = 0; i < lines.size(); i++) {
        SourceLine& line = lines[i];
        if (line.bad) {
            continue;                           
        }

        const std::string& name = line.mnemonic;

        if (name == ".WORD") {
            for (int k = 0; k < line.operands.size(); k++) {
                int value = 0;
                if (!readValue(line.operands[k], line.lineNumber, -32768, 65535, "value", value)) {
                    continue;
                }
                if (!emitWord(line, k, value)) {
                    break;                       
                }
            }
        } else if (name == ".SPACE") {
            for (int k = 0; k < line.size; k++) {
                if (!emitWord(line, k, 0)) {
                    break;
                }
            }
        } else if (name.empty() || name == ".ORG" || name == ".EQU") {
            
        } else {
            OpInfo info;
            opcodes.get(name, info);              
            int word = 0;
            bool ok;
            int expected = operandCountFor(info);
            if (line.operands.size() != expected) {
                addError(line.lineNumber, "wrong number of operands for " + name +
                         " (expected " + numberToString(expected) + ", got " +
                         numberToString(line.operands.size()) + ")");
                ok = false;
            } else if (info.format == FMT_PSEUDO) {
                ok = encodePseudo(line.lineNumber, info, line.operands, word);
            } else {
                ok = encode(line.lineNumber, info, line.operands, word);
            }
            if (ok) {
                emitWord(line, 0, word);
            }
        }

        // lines that produce no word still appear in the listing
        if (name.empty() || name == ".ORG" || name == ".EQU" ||
            (name == ".SPACE" && line.size == 0)) {
            ListingEntry entry;
            entry.lineNumber = line.lineNumber;
            entry.text = line.text;
            listing.pushBack(entry);
        }
    }
}


// Output texts


std::string Assembler::errorText() const {
    std::string result;
    for (int i = 0; i < errors.size(); i++) {
        result += "line " + numberToString(errors[i].line) + ": error: " + errors[i].message + "\n";
    }
    return result;
}

// One line "AA: WWWW" for every assembled word, sorted by address.
std::string Assembler::hexText() const {
    std::string result;
    for (int address = 0; address < MEMORY_SIZE; address++) {
        if (written[address] == 1) {
            result += hexString(address, 2) + ": " + hexString(image[address], 4) + "\n";
        }
    }
    return result;
}

// Text padded with spaces on the left, so numbers line up in columns.
static std::string padLeft(const std::string& text, int width) {
    std::string result = text;
    while ((int)result.length() < width) {
        result.insert(result.begin(), ' ');
    }
    return result;
}

std::string Assembler::listingText() const {
    std::string result;

    // part 1: the listing, one line per source line (or per word)
    for (int i = 0; i < listing.size(); i++) {
        const ListingEntry& entry = listing[i];
        if (entry.hasWord) {
            result += hexString(entry.address, 2) + "  " + hexString(entry.word, 4);
        } else {
            result += "        ";                 // 8 blanks where address and word would be
        }
        result += "  " + padLeft(numberToString(entry.lineNumber), 4) + "  " + entry.text + "\n";
    }

    /* part 2: the symbol table, sorted by name.
    The hash map gives the keys in no useful order, so we sort them with heapSort.*/
    DynamicArray<std::string> names;
    symbols.getAllKeys(names);
    heapSort(names);

    result += "\n";
    for (int i = 0; i < names.size(); i++) {
        int value = 0;
        symbols.get(names[i], value);
        std::string valueText;
        if (value >= 0 && value <= 0xFF) {
            valueText = hexString(value, 2);
        } else {
            valueText = hexString(value & 0xFFFF, 4);
        }
        result += names[i] + " = " + valueText + "\n";
    }
    return result;
}
