#ifndef CODE_GENERATE_H
#define CODE_GENERATE_H

namespace oms
{
    class State;
    class SyntaxTree;

    void CodeGenerate(SyntaxTree *root, State *state);
} // namespace oms

#endif // CODE_GENERATE_H
