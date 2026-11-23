
// Small helper functions used by the assembler, the simulator and the
// debugger: hex printing, parsing numbers, trimming text, reading files.
// They contain no data structure logic, just text handling

#ifndef FORMAT_H
#define FORMAT_H

#include <string>
#include "../ds/DynamicArray.h"

// Number -> text
std::string numberToString(long long value);              // 42 -> "42"
std::string hexString(int value, int digits);             // (10, 2) -> "0A", uppercase

// Text -> number. They return false if the text is not a valid number.
bool parseHexNumber(const std::string& text, int& result);        // "0A" or "0x0A"
bool parseDecimalNumber(const std::string& text, long& result);   // "123" (no sign)

// Text helpers
std::string toUpper(const std::string& text);             // "add" -> "ADD"
std::string trim(const std::string& text);                // remove spaces/tabs at both ends
bool isLetterOrUnderscore(char c);
bool isDigitChar(char c);
bool isIdentifierChar(char c);                            // letter, digit or _

// Cut a text into lines (works for \n and \r\n). The line breaks are removed
void splitLines(const std::string& text, DynamicArray<std::string>& lines);

// Whole-file helpers, they return false if the file cannot be opened
bool readFileToString(const std::string& path, std::string& content);
bool writeStringToFile(const std::string& path, const std::string& content);

#endif
