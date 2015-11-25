#include <iostream>
#include <cstdint>
#include "A.h"

//class A{
//public:
//    virtual void f();
//    A(int32_t a);
//private:
//    int32_t a;
//    void g();
//};

A::A(int32_t a){
    this->a = a;
}

void A::f(){
    a += 1;
    g();
}

void A::g(){
    a += 1;
    std::cout << a << std::endl;
}