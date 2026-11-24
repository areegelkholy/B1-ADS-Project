// Expression.cpp
// Shunting-yard evaluation of constant expressions (see Expression.h).

#include "Expression.h"
#include "Format.h"
#include "../ds/Stack.h"
#include "../ds/Queue.h"

// What kind of piece an expression token is.
const int TOKEN_NUMBER = 0;
const int TOKEN_OPERATOR = 1;
const int TOKEN_LEFT_PAREN = 2;
const int TOKEN_RIGHT_PAREN = 3;

// One piece of the expression.
struct ExprToken {
    int kind;
    long long number;   // used when kind == TOKEN_NUMBER
    char op;            // used when kind == TOKEN_OPERATOR: + - * / % or 'u' (unary minus)
    ExprToken() : kind(TOKEN_NUMBER), number(0), op(' ') {}
};

// Binding strength of an operator: higher binds tighter.
static int precedence(char op) {
    if (op == 'u') return 3;                 // unary minus binds tightest
    if (op == '*' || op == '/' || op == '%') return 2; // followed by multiplication, divison and modulus
    return 1;                                // + and -
}

// Read a number literal (42, 0x2A or 0b101010)
static bool parseNumberLiteral(const std::string& word, long long& value) {
    int base = 10; // decimal base and index
    int start = 0;
    if (word.length() >= 2 && word[0] == '0' && (word[1] == 'x' || word[1] == 'X')) { //hexadecimal
        base = 16;
        start = 2;
    } else if (word.length() >= 2 && word[0] == '0' && (word[1] == 'b' || word[1] == 'B')) { //binary
        base = 2;
        start = 2;
    }
    if ((int)word.length() == start) {
        return false;                        // "0x" with no digits
    }
    long long result = 0;
    for (int i = start; i < (int)word.length(); i++) {
        char c = word[i];
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') { //handles lowercase
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') { //handles uppercase
            digit = c - 'A' + 10;
        } else {
            return false;
        }
        if (digit >= base) {
            return false;                    // for example the 2 in 0b102
        }
        result = result * base + digit;
        if (result > 4294967295LL) {
            return false;                    // far too large
        }
    }
    value = result;
    return true;
}

//  cut the text into tokens. Symbols are replaced by their value here from the symbol table
static bool tokenizeExpression(const std::string& text, const HashMap<int>& symbols,
                               DynamicArray<ExprToken>& tokens, std::string& errorMessage) {
    int i = 0;
    int n = (int)text.length();
    while (i < n) {
        char c = text[i];
        if (c == ' ' || c == '\t') { //ignores white space
            i++;
        } else if (isDigitChar(c)) {
            // a number: take all letters and digits that follow (so 0x2A is one piece)
            int start = i;
            while (i < n && isIdentifierChar(text[i])) {
                i++;
            }
            std::string word = text.substr(start, i - start);
            ExprToken token;
            token.kind = TOKEN_NUMBER;
            if (!parseNumberLiteral(word, token.number)) {
                errorMessage = "malformed number '" + word + "'";
                return false;
            }
            tokens.pushBack(token);
        } else if (isLetterOrUnderscore(c)) {
            // a symbol: look its value up in the symbol table (O(1) average)
            int start = i;
            while (i < n && isIdentifierChar(text[i])) {
                i++;
            }
            std::string name = text.substr(start, i - start);
            int value = 0;
            if (!symbols.get(name, value)) {
                errorMessage = "undefined symbol '" + name + "'";
                return false;
            }
            ExprToken token;
            token.kind = TOKEN_NUMBER;
            token.number = value;
            tokens.pushBack(token);
        } else if (c == '(' || c == ')') {
            ExprToken token;
            token.kind = (c == '(') ? TOKEN_LEFT_PAREN : TOKEN_RIGHT_PAREN;
            tokens.pushBack(token);
            i++;
        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
            ExprToken token;
            token.kind = TOKEN_OPERATOR;
            token.op = c;
            // A minus is UNARY (like in -5) if nothing useful comes before it:
            // at the start, after another operator, or after "("
            if (c == '-') {
                bool nothingBefore = tokens.isEmpty();
                if (!nothingBefore) {
                    const ExprToken& previous = tokens[tokens.size() - 1];
                    nothingBefore = (previous.kind == TOKEN_OPERATOR || previous.kind == TOKEN_LEFT_PAREN);
                }
                if (nothingBefore) {
                    token.op = 'u';
                }
            }
            tokens.pushBack(token);
            i++;
        } else {
            errorMessage = std::string("unexpected character '") + c + "' in expression";
            return false;
        }
    }
    return true;
}

// shunting-yard. Moves the tokens into "output" in postfix order.
static bool toPostfix(const DynamicArray<ExprToken>& tokens, Queue<ExprToken>& output,
                      std::string& errorMessage) {
    Stack<ExprToken> operators;               // operators and "(" that must wait

    for (int i = 0; i < tokens.size(); i++) {
        const ExprToken& token = tokens[i];

        if (token.kind == TOKEN_NUMBER) {
            output.enqueue(token);            // numbers go straight to the output

        } else if (token.kind == TOKEN_LEFT_PAREN) {
            operators.push(token);

        } else if (token.kind == TOKEN_RIGHT_PAREN) {
            // move operators to the output until we find the matching "("
            bool foundOpen = false;
            ExprToken top;
            while (operators.peek(top)) {
                if (top.kind == TOKEN_LEFT_PAREN) {
                    foundOpen = true;
                    break;
                }
                operators.pop(top);
                output.enqueue(top);
            }
            if (!foundOpen) {
                errorMessage = "unbalanced parentheses";
                return false;
            }
            operators.pop(top);               // throw the "(" away

        } else if (token.op == 'u') {
            operators.push(token);            // a prefix operator just waits on the stack

        } else {
            // A binary operator: first move waiting operators that bind at least as
            // tightly to the output (this gives "left to right" for equal precedence).
            ExprToken top;
            while (operators.peek(top) && top.kind == TOKEN_OPERATOR &&
                   precedence(top.op) >= precedence(token.op)) {
                operators.pop(top);
                output.enqueue(top);
            }
            operators.push(token);
        }
    }

    // End of the text: everything still waiting goes to the output.
    ExprToken top;
    while (operators.pop(top)) {
        if (top.kind == TOKEN_LEFT_PAREN) {
            errorMessage = "unbalanced parentheses";
            return false;
        }
        output.enqueue(top);
    }
    return true;
}

//calculate the postfix queue with a stack of values.
static bool calculatePostfix(Queue<ExprToken>& output, long long& result, std::string& errorMessage) {
    Stack<long long> values;
    ExprToken token;

    while (output.dequeue(token)) {
        if (token.kind == TOKEN_NUMBER) {
            values.push(token.number);
        } else if (token.op == 'u') {
            long long a;
            if (!values.pop(a)) {
                errorMessage = "missing operand";
                return false;
            }
            values.push(-a);
        } else {
            long long right;
            long long left;
            // the right operand was pushed last, so it comes off the stack first
            if (!values.pop(right) || !values.pop(left)) {
                errorMessage = "missing operand";
                return false;
            }
            long long answer = 0;
            // Perform the operation using the two operands.
            if (token.op == '+') {
                answer = left + right;
            } else if (token.op == '-') {
                answer = left - right;
            } else if (token.op == '*') {
                answer = left * right;
            } else if (right == 0) {
                // Division and modulus by zero are not allowed.
                errorMessage = "division by zero";
                return false;
            } else if (token.op == '/') {
                answer = left / right;        
            } else {
                answer = left % right;
            }
            values.push(answer);
        }
    }

    // A valid expression must leave exactly one final result on the Stack.
    if (values.size() != 1) {
        errorMessage = values.isEmpty() ? "empty expression" : "malformed expression";
        return false;
    }
    values.pop(result);
    return true;
}
// Main function that evaluates an expression by calling the three stages.
bool evaluateExpression(const std::string& text, const HashMap<int>& symbols,
                        long long& result, std::string& errorMessage) {
    DynamicArray<ExprToken> tokens;
    if (!tokenizeExpression(text, symbols, tokens, errorMessage)) {
        return false;
    }
    Queue<ExprToken> output;
    if (!toPostfix(tokens, output, errorMessage)) {
        return false;
    }
    return calculatePostfix(output, result, errorMessage);
}
