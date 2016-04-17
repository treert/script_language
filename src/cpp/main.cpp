#include<iostream>
#include<cstdio>
#include<string>
#include<stdint.h>
#include "windows.h"

using namespace std;

class D
{
public:
    D(int a){ cout << __FUNCTION__ << a << endl; };
    ~D(){ cout << __FUNCTION__ << endl; };

private:

};

class B:D
{
public:
    B():D(2){ cout << __FUNCTION__ << endl; };
    ~B(){ cout << __FUNCTION__ << endl; };

private:

};

int main(){
    //A a;
    B b;
    b.~B();
}