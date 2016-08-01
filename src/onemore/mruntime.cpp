#include "mruntime.h"
#include "mupvalue.h"

namespace oms
{
    Stack::Stack()
        : stack_(kBaseStackSize),
          top_(nullptr)
    {
        top_ = &stack_[0];
    }

    void Stack::SetNewTop(Value *top)
    {
        Value *old = top_;
        top_ = top;
        top_->SetNil();

        // Clear values between new top to old
        for (; top <= old; ++top)
            top->SetNil();
    }

    void Stack::CloseUpvalueTo(Value *ptr)
    {
        while (!upvalue_list_.empty())
        {
            auto upvalue = upvalue_list_.back();
            if (upvalue->GetValue() <= ptr)
            {
                upvalue->Close();
                upvalue_list_.pop_back();
            }
        }
    }

    CallInfo::CallInfo()
        : register_(nullptr),
          func_(nullptr),
          instruction_(nullptr),
          end_(nullptr)
    {
    }
} // namespace oms
