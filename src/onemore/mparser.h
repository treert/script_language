#ifndef PARSER_H
#define PARSER_H

#include "mSyntaxTree.h"
#include <memory>

namespace oms
{
    class Lexer;
    class State;

    std::unique_ptr<SyntaxTree> Parse(Lexer *lexer);
} // namespace oms

#endif // PARSER_H
