#include<iostream>
#include<cstdio>
#include<string>
#include<stdint.h>
#include "A.h"

using namespace std;

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

    A x(10);

    x.f();

}