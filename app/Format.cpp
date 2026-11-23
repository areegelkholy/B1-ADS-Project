
#include "Format.h"
#include <fstream>

std::string numberToString(long long value) {
    return std::to_string(value);
}

// Builds the text digit by digit, from the right, using the "0123456789ABCDEF" table.
std::string hexString(int value, int digits) {
    const char* table = "0123456789ABCDEF";
    unsigned int v = (unsigned int)value;
    std::string result;
    for (int i = 0; i < digits; i++) {
        result.insert(result.begin(), table[v % 16]);
        v = v / 16;
    }
    // if the number needs more digits than requested, keep the extra ones
    while (v > 0) {
        result.insert(result.begin(), table[v % 16]);
        v = v / 16;
    }
    return result;
}

bool parseHexNumber(const std::string& text, int& result) {
    std::string t = trim(text);
    int start = 0;
    if (t.length() >= 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
        start = 2;                       // skip the "0x" prefix
    }
    if ((int)t.length() == start || (int)t.length() - start > 8) {
        return false;                    // no digits, or too many for an int
    }
    long value = 0;
    for (int i = start; i < (int)t.length(); i++) {
        char c = t[i];
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return false;
        }
        value = value * 16 + digit;
    }
    result = (int)value;
    return true;
}

bool parseDecimalNumber(const std::string& text, long& result) {
    std::string t = trim(text);
    if (t.empty() || t.length() > 9) {
        return false;
    }
    long value = 0;
    for (int i = 0; i < (int)t.length(); i++) {
        if (!isDigitChar(t[i])) {
            return false;
        }
        value = value * 10 + (t[i] - '0');
    }
    result = value;
    return true;
}

std::string toUpper(const std::string& text) {
    std::string result = text;
    for (int i = 0; i < (int)result.length(); i++) {
        if (result[i] >= 'a' && result[i] <= 'z') {
            result[i] = (char)(result[i] - 'a' + 'A');
        }
    }
    return result;
}

std::string trim(const std::string& text) {
    int start = 0;
    int end = (int)text.length();          // one past the last character
    while (start < end && (text[start] == ' ' || text[start] == '\t' || text[start] == '\r' || text[start] == '\n')) {
        start++;
    }
    while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r' || text[end - 1] == '\n')) {
        end--;
    }
    return text.substr(start, end - start);
}

bool isLetterOrUnderscore(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isDigitChar(char c) {
    return c >= '0' && c <= '9';
}

bool isIdentifierChar(char c) {
    return isLetterOrUnderscore(c) || isDigitChar(c);
}

void splitLines(const std::string& text, DynamicArray<std::string>& lines) {
    std::string current;
    for (int i = 0; i < (int)text.length(); i++) {
        char c = text[i];
        if (c == '\n') {
            lines.pushBack(current);
            current = std::string();
        } else if (c != '\r') {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        lines.pushBack(current);         // last line without a final line break
    }
}

bool readFileToString(const std::string& path, std::string& content) {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
        return false;
    }
    content = std::string();
    char c;
    while (file.get(c)) {
        content.push_back(c);
    }
    return true;
}

bool writeStringToFile(const std::string& path, const std::string& content) {
    std::ofstream file(path.c_str(), std::ios::out | std::ios::binary);
    if (!file) {
        return false;
    }
    file << content;
    return true;
}
