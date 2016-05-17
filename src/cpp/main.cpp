#include<iostream>
#include<cstdio>
#include<string>
#include<vector>
#include<stdint.h>
#include "windows.h"

using namespace std;

class D
{
public:
    D(int a,float b=8.1){ cout << __FUNCTION__ << a << b<< endl; }
};



int main(){
    D a(1);
    D aa(1, 2);
    D aaa{ 1};
    D aaaa{ 1, 2 };
}