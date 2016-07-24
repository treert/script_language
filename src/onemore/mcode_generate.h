#ifndef CODE_GENERATE_H
#define CODE_GENERATE_H

#include "m-visitor.h"
#include <memory>

namespace oms
{
    class State;

    void CodeGenerate(SyntaxTree *root, State *state);
} // namespace oms

#endif // CODE_GENERATE_H
