#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

namespace oms
{
    class State;
    class SyntaxTree;

    void SemanticAnalysis(SyntaxTree *root, State *state);
}

#endif // SEMANTIC_ANALYSIS_H
