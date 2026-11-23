
/* The two-pass assembler: TISA-16 assembly text -> machine words.

       PASS 1: walk through the program, count how many words each line
               needs, and store every label in the symbol table. This is why forward references work: after pass 1
               every label has an address.
       PASS 2: walk through the program again and encode every instruction
               into its 16-bit word, now that all symbols are known.
               Expressions are evaluated with the shunting-yard code in
               Expression.cpp.

     Data structures used here:
       HashMap<int>    symbols    label / .EQU name -> value       
       HashMap<OpInfo> opcodes    mnemonic -> how to encode it     
       DynamicArray    lines, image memory, listing entries, errors
       heapSort        sorts the error messages by line number and the
                       symbol table by name (listing file)

     The assembler never prints and never touches files, so it is easy to
     test.*/


#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include "../ds/DynamicArray.h"
#include "../ds/HashMap.h"
#include "Isa.h"
#include "Tokenizer.h"

// One error message. Errors are sorted by line number before they are shown.
struct AsmError {
    int line;
    int order;            // 0, 1, 2 ... in the order they were found (breaks ties)
    std::string message;
    AsmError() : line(0), order(0) {}
    bool operator<(const AsmError& other) const {
        if (line != other.line) {
            return line < other.line;
        }
        return order < other.order;
    }
};

// One line of the listing file.
struct ListingEntry {
    int address;
    int word;
    bool hasWord;    
    int lineNumber;
    std::string text;     
    ListingEntry() : address(0), word(0), hasWord(false), lineNumber(0) {}
};

class Assembler {
private:
    HashMap<int> symbols;             
    HashMap<OpInfo> opcodes;          
    DynamicArray<SourceLine> lines;   // the tokenized program
    DynamicArray<int> image;          // the 256 memory words that are produced
    DynamicArray<int> written;        // 1 if image[address] was assembled, else 0
    DynamicArray<ListingEntry> listing;
    DynamicArray<AsmError> errors;
    int errorOrder;
    int wordsWritten;

    // errors
    void addError(int lineNumber, const std::string& message);

    // pass 1
    void pass1();
    void defineSymbol(const std::string& name, long long value, int lineNumber);
    bool checkOperandCount(SourceLine& line, int expected);
    void pass1Directive(SourceLine& line, int& location);

    // pass 2
    void pass2();
    bool emitWord(const SourceLine& line, int index, int word);
    bool encode(int lineNumber, const OpInfo& info, const DynamicArray<std::string>& ops, int& word);
    bool encodePseudo(int lineNumber, const OpInfo& info, const DynamicArray<std::string>& ops, int& word);

    // small helpers used by both passes
    bool readRegister(const std::string& text, int lineNumber, int& reg);
    bool readValue(const std::string& text, int lineNumber, int minValue, int maxValue,
                   const char* what, int& value);
    static bool splitMemoryOperand(const std::string& text, std::string& offsetText, std::string& regText);

public:
    Assembler();

    // Assemble the whole source text. Returns true if there were no errors.
    bool assemble(const std::string& sourceText);

    int errorCount() const { return errors.size(); }
    std::string errorText() const;    
    std::string hexText() const;      
    std::string listingText() const;  
    int wordCount() const { return wordsWritten; }

    bool lookupSymbol(const std::string& name, int& value) const { return symbols.get(name, value); }
};

#endif
