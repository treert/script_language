#pragma once
// author:onemore
// this is a template .h file

#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#define MY_NAME_SPACE_BEGIN namespace MyNameSpace {
#define MY_NAME_SPACE_END   }
#define DISALLOW_COPY_AND_ASSIGN(NAME) \
    NAME(const NAME&other) = delete;\
    NAME& operator =(const NAME&other) = delete;

class PtrClass;

MY_NAME_SPACE_BEGIN

class IMyInterface
{
public:
    enum MyErrors{
        My_Error_Out_Of_Memory = 1,
        My_Errir_Wrong_Param = 2,
    };
    virtual void Func() = 0;
};

class MyClass :public IMyInterface
{
public:
    typedef std::vector<int> IntVector;

    explicit MyClass(int iId);

    virtual void Func() override;
    int GetId()const{ return _iId; }
    void SetId(int iId){ _iId = iId; }
private:
    DISALLOW_COPY_AND_ASSIGN(MyClass);

    void _SpecialFunc(int iId,const IntVector &vIds,PtrClass *pPtr);
    const int k_iDaysInWeek = 7;

    int32_t _iId;
    std::string _sName;
    PtrClass* _pPtrClass;
    IntVector _vIds;
};

MY_NAME_SPACE_END