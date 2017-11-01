#include "hello.h"
#include<iostream>
using namespace std;
namespace hello{
    void show()
    {
        cout << "hello world" << endl;
    }

    MyClass::MyClass()
    {
        cout << "Constructor MyClass" << endl;
    }

    MyClass::~MyClass()
    {
        cout << "Destructor MyClass" << endl;
    }

    void MyClass::DoSomething()
    {
        cout << "Do Something" << endl;
    }
}
