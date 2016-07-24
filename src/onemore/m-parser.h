#ifndef PARSER_H
#define PARSER_H

#include "msyntax_tree.h"
#include <memory>

namespace oms
{
    class Lexer;

    std::unique_ptr<SyntaxTree> Parse(Lexer *lexer);
} // namespace oms

#endif // PARSER_H
