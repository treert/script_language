#include "mupvalue.h"

namespace oms
{
    void Upvalue::Accept(GCObjectVisitor *v)
    {
        if (v->Visit(this))
        {
            (*ptr_value_).Accept(v);
        }
    }
} // namespace oms
