#include <iostream>

struct A
{
};

struct B
{
};

class C: A, public B
{
};

struct AA : A
{
};

struct BB : AA
{
};

struct CC : A
{
};

template <typename T>
struct ZZ : T
{
};

struct ZZZ : ZZ<const A>
{
};
struct ZZZZ : ZZ<A>
{
};

template struct ZZ<const A>;

struct DD : CC
{
};

struct EE : AA, A, CC
{
};

template<typename T>
struct D: protected T
{
};

template<typename T>
struct F
{
};

template<typename T>
struct E: F<T>
{
    F<A> d;
};

template<typename T>
struct G: E<T>
{
    virtual void thing(G<T> g)
    {
    }
};

int main()
{
    D<A> d;
    E<A> e;
    G<A> g;
    G<B> gg;
}
