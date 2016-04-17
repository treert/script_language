#ifndef UPVALUE_H
#define UPVALUE_H

#include "GC.h"
#include "Value.h"

namespace luna
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
} // namespace luna

#endif // UPVALUE_H
