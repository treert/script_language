#include<iostream>
#include<cstdio>
#include<string>
#include<stdint.h>

using namespace std;

enum class A :int32_t{
    A=1,
    B=2
};

enum class B{
    A,
    B
};

double a, b;

void f(void*xx)
{
    char*c = (char*)xx;
    c[7] = 0xff;
    c[6] = 0xff;
    c[5] = 0x0;
}

int main(){
    f(&a);
    f(&b);

    cout << a << endl;
    cout << b << endl;
    cout << (a == b) << endl;
}