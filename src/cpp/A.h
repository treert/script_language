#pragma once
#include <cstdint>

class A{
public:
    static const int32_t c = 124;
    virtual void f();
    A(int32_t a);
private:
    int32_t a = 3;
    void g();
};