
#include "Tokenizer.h"
#include "Format.h"

bool isIdentifier(const std::string& text) {
    if (text.empty() || !isLetterOrUnderscore(text[0])) {
        return false;
    }
    for (int i = 1; i < (int)text.length(); i++) {
        if (!isIdentifierChar(text[i])) {
            return false;
        }
    }
    return true;
}

// Remove every space and tab inside the text
static std::string removeSpaces(const std::string& text) {
    std::string result;
    for (int i = 0; i < (int)text.length(); i++) {
        if (text[i] != ' ' && text[i] != '\t' && text[i] != '\r') {
            result.push_back(text[i]);
        }
    }
    return result;
}

void tokenizeLine(const std::string& text, int lineNumber, SourceLine& out) {
    out = SourceLine();                       
    out.lineNumber = lineNumber;
    out.text = text;

    // 1) Cut off the comment: everything after the first ';'.
    std::string code = text;
    int semicolon = (int)code.find(';');
    if (semicolon != (int)std::string::npos) {
        code = code.substr(0, semicolon);
    }

    // 2) A label is the text before the first ':'.
    int colon = (int)code.find(':');
    if (colon != (int)std::string::npos) {
        std::string labelText = trim(code.substr(0, colon));
        if (isIdentifier(labelText)) {
            out.label = labelText;           // labels are case-sensitive
        } else {
            out.error = "invalid label '" + labelText + "'";
        }
        code = code.substr(colon + 1);
    }

    // 3) The mnemonic is the first word that is left.
    code = trim(code);
    if (code.empty()) {
        return;                               
    }
    int end = 0;
    while (end < (int)code.length() && code[end] != ' ' && code[end] != '\t') {
        end++;
    }
    out.mnemonic = toUpper(code.substr(0, end));

    // 4) Everything after the mnemonic is the operand list, separated by commas.
    std::string rest = trim(code.substr(end));
    if (rest.empty()) {
        return;                              
    }
    std::string current;
    for (int i = 0; i < (int)rest.length(); i++) {
        if (rest[i] == ',') {
            out.operands.pushBack(removeSpaces(current));
            current = std::string();
        } else {
            current.push_back(rest[i]);
        }
    }
    out.operands.pushBack(removeSpaces(current));   
}
