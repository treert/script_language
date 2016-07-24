#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include "msyntax_tree.h"

namespace oms
{
    class State;

    void SemanticAnalysis(SyntaxTree *root, State *state);
}

#endif // SEMANTIC_ANALYSIS_H