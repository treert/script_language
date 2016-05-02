#pragma once

#include "msyntax_tree.h"
#include <memory>

namespace oms
{
    class Lexer;
    class State;

    std::unique_ptr<SyntaxTree> Parse(Lexer *lexer);
} // namespace oms
