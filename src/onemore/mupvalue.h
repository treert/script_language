#ifndef UPVALUE_H
#define UPVALUE_H

#include "mgc.h"
#include "m-value.h"

namespace oms
{
    class Upvalue : public GCObject
    {
    public:
        virtual void Accept(GCObjectVisitor *v);

        void SetValue(const Value &value)
        { value_ = value; }

        Value * GetValue()
        { return &value_; }

    private:
        Value value_;
    };
} // namespace oms

#endif // UPVALUE_H
