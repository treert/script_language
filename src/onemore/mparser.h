#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include "msyntax_tree.h"

namespace oms
{
    class Lexer;
    class SyntaxTree;

    std::unique_ptr<SyntaxTree> Parse(Lexer *lexer);
} // namespace oms

#endif // PARSER_H
