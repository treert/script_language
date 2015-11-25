#pragma once
#include <cstdint>

class A{
public:
    virtual void f();
    A(int32_t a);
private:
    int32_t a;
    void g();
};