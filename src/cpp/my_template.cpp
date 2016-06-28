//author:onemore
// this is a template cpp file
#include "my_template.h"

#include <assert.h>

MyNameSpace::MyClass::MyClass(int iId) :
_iId(iId),
_pPtrClass(nullptr)
{
}

void MyNameSpace::MyClass::Func()
{
}

static void local_handle_func()
{
}

void MyNameSpace::MyClass::_SpecialFunc(int iId, const IntVector &vIds, PtrClass *pPtr)
{
    assert(iId > 0);
    assert(nullptr != pPtr);

    for (auto id : vIds)
    {
        // todo(onemore)
    }
    local_handle_func();
}