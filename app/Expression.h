// Expression.h
// ---------------------------------------------------------------
// Evaluates constant expressions
// with the SHUNTING-YARD algorithm. This is where our Stack and Queue
// are used in the assembler.
//
// Allowed: numbers, symbols,
// operators, parentheses and unary minus.
//
// HOW IT WORKS (two phases):
//   1) Shunting-yard turns the normal ("infix") text into "postfix" order,
//      where the operator comes after its operands:   2 + 3 * 4  ->  2 3 4 * +
//         - an operator STACK holds operators,
//         - an output QUEUE collects the numbers and operators in postfix order.
//   2) We read the output Queue from beginning to end and use a value Stack to calculate the
//      expression. When we encounter a number, we push it onto the Stack. When we encounter an
//       operator, we take the required numbers from the Stack, perform the operation, and push the     result back onto the Stack. We repeat this until we have one final answer.
// ---------------------------------------------------------------

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include "../ds/HashMap.h"

// Calculate the value of "text". Symbols are looked up in "symbols".
// Returns true and sets "result" if the expression is fine.
// Returns false and sets "errorMessage" if not
bool evaluateExpression(const std::string& text,
                        const HashMap<int>& symbols,
                        long long& result,
                        std::string& errorMessage);

#endif
