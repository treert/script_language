#include <iostream>
#include <cstdint>
//#include "A.h"

class A{
public:
    static const int32_t c = 123;
    virtual void f();
    A(int32_t a);
private:
    int32_t a = 1;
    void g();
};
A::A(int32_t a){
    this->a = a;
}

void A::f(){
    a += 1;
    g();
}

void A::g(){
    a += 1;
    std::cout << c << std::endl;
    
}

//const int32_t A::c = 12;