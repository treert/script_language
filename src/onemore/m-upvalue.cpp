#include "m-upvalue.h"

namespace oms
{
    void Upvalue::Accept(GCObjectVisitor *v)
    {
        if (v->Visit(this))
        {
            value_.Accept(v);
        }
    }
} // namespace oms
