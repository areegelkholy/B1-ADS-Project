
/* Everything that describes the TISA-16 instruction set in one place,
   so the assembler (text -> numbers) and the simulator(numbers -> text) agree with each other.

     - OpInfo + buildOpcodeTable():  mnemonic -> how to encode it  (a HashMap)
     - parseRegister():              "R5" -> 5
     - disassemble():                16-bit word -> "ADD R1, R2, R0"
     - helpers 
  */

#ifndef ISA_H
#define ISA_H

#include <string>
#include "../ds/HashMap.h"

//  Machine constants 
const int MEMORY_SIZE = 256;       
const int MAX_ADDRESS = 0xFF;
const int STACK_START = 0xFC;      
const int VECTOR_TABLE = 0xFC;     
const int REGISTER_COUNT = 8;

// Field ranges 
const int IMM6_MIN = -32;
const int IMM6_MAX = 31;
const int IMM9_MIN = -256;
const int IMM9_MAX = 255;

// The instruction "shapes". They decide how the operands areread and packed into the 16-bit word.
const int FMT_R = 0;        
const int FMT_ADDI = 1;     
const int FMT_LDI = 2;      
const int FMT_MEM = 3;      
const int FMT_CMP = 4;      
const int FMT_BRANCH = 5;   
const int FMT_CALL = 6;     
const int FMT_RET = 7;      
const int FMT_PUSH = 8;     
const int FMT_POP = 9;      
const int FMT_SYS = 10;     
const int FMT_OUT = 11;     
const int FMT_PSEUDO = 12;  

// What the opcode table stores for one mnemonic.
struct OpInfo {
    int opcode;   
    int format;   
    int extra;    
    OpInfo() : opcode(0), format(0), extra(0) {}
    OpInfo(int o, int f, int e) : opcode(o), format(f), extra(e) {}
};

// Pseudo-instruction numbers 
const int PSEUDO_MOV = 0;
const int PSEUDO_INC = 1;
const int PSEUDO_DEC = 2;
const int PSEUDO_CLR = 3;

// Fill the opcode table (real instructions and pseudo-instructions).
void buildOpcodeTable(HashMap<OpInfo>& table);

// How many operands does this instruction take?
int operandCountFor(const OpInfo& info);

// "R0".."R7" (any case) -> 0..7. Returns false for anything else.
bool parseRegister(const std::string& text, int& reg);

// Sign extension: take the lowest "bits" bits of value as a two's-complement number.
int signExtend(int value, int bits);

// A 16-bit register value (0..65535) as a signed number (-32768..32767).
int toSigned16(int value);

// Turn an instruction word into text. Only real instructions are printed (never MOV, INC).
std::string disassemble(int word);

#endif
