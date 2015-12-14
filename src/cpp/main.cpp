#include<iostream>
#include<cstdio>
#include<string>
#include<stdint.h>
#include "A.h"

using namespace std;
int main(){
    puts("no std");
    double d = 1.234567890123456789;
    printf("%g\n", d);
    printf("%.30g\n", d);
    printf("%.30f\n", d);

    cout << A::c << endl;
    A a(1);
    a.f();
}