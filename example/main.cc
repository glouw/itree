#include <iostream>
#include <utility>

struct Point
{
    int x;
    int y;
};

struct A
{
};

struct Base : A
{
protected:
    void test()
    {
    }
};

class X: Base, A
{
    void test()
    {
    }

    void other()
    {
    }

    void thing()
    {
    }

public:
    template <int... T>
    void fun()
    {
    }
};

class YY : X
{
};

struct Y
{
    template <Point... T>
    void fun()
    {
    }
};

int main()
{
    X x;
    x.fun<1,2,3,4,5,6>();

    Y y;
    y.fun<{1,2}, {2,3}>();
}
