#ifndef RUNTIME_H
#define RUNTIME_H

#include "mvalue.h"
#include <vector>
#include <list>

namespace oms
{
    class Closure;
    struct Instruction;

    // Runtime stack, registers of each function is one part of stack.
    struct Stack
    {
        static const int kBaseStackSize = 10000;

        std::vector<Value> stack_;
        Value *top_;
        // open upvalues list
        std::list<Upvalue*> upvalue_list_;

        Stack();
        Stack(const Stack&) = delete;
        void operator = (const Stack&) = delete;

        // Set new top pointer, and [new top, old top) will be set nil
        void SetNewTop(Value *top);

        // close upvalues to ptr
        void CloseUpvalueTo(Value *ptr);
    };

    // Function call stack info
    struct CallInfo
    {
        // register base pointer which points to Stack
        Value *register_;
        // current closure, pointer to stack Value
        Value *func_;
        // current Instruction
        const Instruction *instruction_;
        // Instruction end
        const Instruction *end_;
        // expect result of this function call
        int expect_result_;

        CallInfo();
    };
} // namespace oms

#endif // RUNTIME_H
