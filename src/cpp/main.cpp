#include<iostream>
#include<cstdio>
#include<string>
#include<stdint.h>
#include "windows.h"

using namespace std;

class D
{
private:
    D() { cout << __FUNCTION__ << endl; };
public:
    ~D(){ cout << __FUNCTION__ << endl; };
protected:
    static void xxx(){ cout << __FUNCTION__ << endl; };

private:

};

class B:public D
{
public:
    static void abc(){ cout << __FUNCTION__ << endl; }

private:

};

int main(){
    //A a;
    B::abc();
    //b.~B();
}