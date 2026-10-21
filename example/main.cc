#include <iostream>

struct A {
    virtual void foo() {
        std::cout << "A::foo\n";
    }

    virtual ~A() = default;
};

struct B : virtual A {
    void foo() override {
        std::cout << "B::foo\n";
    }
};

struct C : virtual A {
    void foo() override {
        std::cout << "C::foo\n";
    }
};

struct D : B, C {
    void foo() override {
        std::cout << "D::foo\n";
    }
};

struct E : virtual A {
    void foo() override {
        std::cout << "E::foo\n";
    }
};

struct F : D, E {
    void foo() override {
        std::cout << "F::foo\n";
    }
};

int main() {
    F obj;

    A* a = &obj;
    B* b = &obj;
    C* c = &obj;
    D* d = &obj;
    E* e = &obj;
    F* f = &obj;

    a->foo();
    b->foo();
    c->foo();
    d->foo();
    e->foo();
    f->foo();
}
