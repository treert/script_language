#pragma once

namespace oms
{
    class State;
    class SyntaxTree;

    void SemanticAnalysis(SyntaxTree *root, State *state);
}