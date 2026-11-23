
/*Step 1 of the assembler: split ONE source line into its parts.

         loop:   ADD  R1, R2, R3     ; add them
         label   mnemonic operands   comment (thrown away)

     Every part is optional. The tokenizer does NOT check if the mnemonic
     exists or if the operands make sense: the assembler does that.*/


#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include "../ds/DynamicArray.h"


struct SourceLine {
    int lineNumber;                    
    std::string text;                   
    std::string label;                  
    std::string mnemonic;              
    DynamicArray<std::string> operands; 
    std::string error;                  

    // filled in by the assembler
    int address;                        // where the first word of this line goes
    int size;                           // how many words this line produces
    bool bad;                           // true if pass 1 found an error on this line

    SourceLine() : lineNumber(0), address(0), size(0), bad(false) {}
};

bool isIdentifier(const std::string& text);

// Split one line of source code into label, mnemonic and operands. "out" is overwritten completely.
void tokenizeLine(const std::string& text, int lineNumber, SourceLine& out);

#endif
