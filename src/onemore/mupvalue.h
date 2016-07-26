#ifndef UPVALUE_H
#define UPVALUE_H

#include "mgc.h"
#include "mvalue.h"

namespace oms
{
    class Upvalue : public GCObject
    {
    public:
        Upvalue() = default;
        virtual void Accept(GCObjectVisitor *v);

        void Close()
        {
            value_ = *ptr_value_;
            ptr_value_ = &value_;
        }

        void SetValuePtr(Value *ptr)
        {
            ptr_value_ = ptr;
        }

        void SetValue(const Value &value)
        { value_ = value; }

        Value * GetValue()
        { return ptr_value_; }

    private:
        Value value_;
        Value *ptr_value_ = nullptr;
    };
} // namespace oms

#endif // UPVALUE_H
