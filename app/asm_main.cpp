//   -> writes program.hex and program.lst
//   -> prints every error as "line <n>: error: <message>" and writes no files if there was any error.

#include <iostream>
#include "Assembler.h"
#include "Format.h"

static std::string baseName(const std::string& path) {
    int length = (int)path.length();
    if (length > 4 && toUpper(path.substr(length - 4)) == ".ASM") {
        return path.substr(0, length - 4);
    }
    return path;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "usage: asm program.asm" << std::endl;
        return 2;
    }
    std::string path = argv[1];
    std::string source;
    if (!readFileToString(path, source)) {
        std::cout << "cannot open '" << path << "'" << std::endl;
        return 2;
    }

    Assembler assembler;
    if (!assembler.assemble(source)) {
        std::cout << assembler.errorText();
        std::cout << assembler.errorCount() << " error(s), no output written" << std::endl;
        return 1;
    }

    std::string base = baseName(path);
    if (!writeStringToFile(base + ".hex", assembler.hexText()) ||
        !writeStringToFile(base + ".lst", assembler.listingText())) {
        std::cout << "cannot write the output files" << std::endl;
        return 2;
    }
    std::cout << "assembled " << assembler.wordCount() << " words, wrote "
              << base << ".hex and " << base << ".lst" << std::endl;
    return 0;
}
