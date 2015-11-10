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


int main(){
    cout << (int32_t)A::A << endl;
    cout << (int32_t)A::B << endl;
    cout << (int32_t)A::A << endl;
    cout << (int32_t)A::B << endl;
}