// Implementation of the instruction-set architecture

#include "Isa.h"
#include "Format.h"

void buildOpcodeTable(HashMap<OpInfo>& table) {
    // opcode, format, extra
    table.put("ADD",  OpInfo(0x0, FMT_R, 0));
    table.put("SUB",  OpInfo(0x1, FMT_R, 0));
    table.put("AND",  OpInfo(0x2, FMT_R, 0));
    table.put("OR",   OpInfo(0x3, FMT_R, 0));
    table.put("XOR",  OpInfo(0x4, FMT_R, 0));
    table.put("ADDI", OpInfo(0x5, FMT_ADDI, 0));
    table.put("LDI",  OpInfo(0x6, FMT_LDI, 0));
    table.put("LW",   OpInfo(0x7, FMT_MEM, 0));
    table.put("SW",   OpInfo(0x8, FMT_MEM, 0));
    table.put("CMP",  OpInfo(0x9, FMT_CMP, 0));

    // All branches use opcode 0xA
    table.put("JMP",  OpInfo(0xA, FMT_BRANCH, 0));
    table.put("BEQ",  OpInfo(0xA, FMT_BRANCH, 1));
    table.put("BNE",  OpInfo(0xA, FMT_BRANCH, 2));
    table.put("BLT",  OpInfo(0xA, FMT_BRANCH, 3));
    table.put("BGE",  OpInfo(0xA, FMT_BRANCH, 4));
    table.put("BGT",  OpInfo(0xA, FMT_BRANCH, 5));
    table.put("BLE",  OpInfo(0xA, FMT_BRANCH, 6));

    table.put("CALL", OpInfo(0xB, FMT_CALL, 0));
    table.put("RET",  OpInfo(0xC, FMT_RET, 0));
    table.put("PUSH", OpInfo(0xD, FMT_PUSH, 0));
    table.put("POP",  OpInfo(0xE, FMT_POP, 0));

    // System instructions: opcode 0xF
    table.put("NOP",  OpInfo(0xF, FMT_SYS, 0));
    table.put("HALT", OpInfo(0xF, FMT_SYS, 1));
    table.put("OUT",  OpInfo(0xF, FMT_OUT, 2));
    table.put("EI",   OpInfo(0xF, FMT_SYS, 3));
    table.put("DI",   OpInfo(0xF, FMT_SYS, 4));
    table.put("RETI", OpInfo(0xF, FMT_SYS, 5));

    // Pseudo-instructions 
    table.put("MOV",  OpInfo(0, FMT_PSEUDO, PSEUDO_MOV));
    table.put("INC",  OpInfo(0, FMT_PSEUDO, PSEUDO_INC));
    table.put("DEC",  OpInfo(0, FMT_PSEUDO, PSEUDO_DEC));
    table.put("CLR",  OpInfo(0, FMT_PSEUDO, PSEUDO_CLR));
}

int operandCountFor(const OpInfo& info) {
    switch (info.format) {
        case FMT_R:      return 3;
        case FMT_ADDI:   return 3;
        case FMT_LDI:    return 2;
        case FMT_MEM:    return 2;
        case FMT_CMP:    return 2;
        case FMT_BRANCH: return 1;
        case FMT_CALL:   return 1;
        case FMT_RET:    return 0;
        case FMT_PUSH:   return 1;
        case FMT_POP:    return 1;
        case FMT_SYS:    return 0;
        case FMT_OUT:    return 1;
        case FMT_PSEUDO: return info.extra == PSEUDO_MOV ? 2 : 1;
    }
    return 0;
}

bool parseRegister(const std::string& text, int& reg) {
    if (text.length() != 2) {
        return false;
    }
    if (text[0] != 'R' && text[0] != 'r') {
        return false;
    }
    if (text[1] < '0' || text[1] > '7') {
        return false;
    }
    reg = text[1] - '0';
    return true;
}

int signExtend(int value, int bits) {
    int mask = (1 << bits) - 1;
    value = value & mask;                  
    if (value >= (1 << (bits - 1))) {      
        value = value - (1 << bits);
    }
    return value;
}

int toSigned16(int value) {
    value = value & 0xFFFF;
    if (value >= 0x8000) {
        value = value - 0x10000;
    }
    return value;
}

// Text of register number r: 3 -> "R3".
static std::string regName(int r) {
    return std::string("R") + numberToString(r);
}

std::string disassemble(int word) {
    word = word & 0xFFFF;
    int opcode = (word >> 12) & 0xF;
    int rd = (word >> 9) & 7;
    int rs = (word >> 6) & 7;
    int rt = (word >> 3) & 7;

    static const char* rNames[5] = {"ADD", "SUB", "AND", "OR", "XOR"};
    static const char* branchNames[7] = {"JMP", "BEQ", "BNE", "BLT", "BGE", "BGT", "BLE"};
    static const char* sysNames[6] = {"NOP", "HALT", "OUT", "EI", "DI", "RETI"};

    if (opcode <= 4) {
        return std::string(rNames[opcode]) + " " + regName(rd) + ", " + regName(rs) + ", " + regName(rt);
    }
    switch (opcode) {
        case 0x5:
            return "ADDI " + regName(rd) + ", " + regName(rs) + ", " + numberToString(signExtend(word, 6));
        case 0x6:
            return "LDI " + regName(rd) + ", " + numberToString(signExtend(word, 9));
        case 0x7:
            return "LW " + regName(rd) + ", " + numberToString(signExtend(word, 6)) + "(" + regName(rs) + ")";
        case 0x8:
            return "SW " + regName(rd) + ", " + numberToString(signExtend(word, 6)) + "(" + regName(rs) + ")";
        case 0x9:
            return "CMP " + regName(rs) + ", " + regName(rt);
        case 0xA: {
            int cond = rd;                       // the condition sits in bits 11..9
            if (cond > 6) {
                return "invalid branch condition " + numberToString(cond);
            }
            return std::string(branchNames[cond]) + " 0x" + hexString(word & 0xFF, 2);
        }
        case 0xB:
            return "CALL 0x" + hexString(word & 0xFF, 2);
        case 0xC:
            return "RET";
        case 0xD:
            return "PUSH " + regName(rs);
        case 0xE:
            return "POP " + regName(rd);
        default: {                                // 0xF: system instructions
            int func = word & 7;
            if (func > 5) {
                return "invalid system function " + numberToString(func);
            }
            if (func == 2) {
                return "OUT " + regName(rs);
            }
            return sysNames[func];
        }
    }
}
