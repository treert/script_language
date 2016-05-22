#ifndef CODE_GENERATE_H
#define CODE_GENERATE_H

#include "mVisitor.h"
#include <memory>

namespace oms
{
    class State;

    void CodeGenerate(SyntaxTree *root, State *state);
} // namespace oms

#endif // CODE_GENERATE_H
