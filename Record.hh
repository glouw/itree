#pragma once

#include "clang/AST/DeclCXX.h"

#include <string>
#include <set>
#include <vector>

struct Base
{
    std::string name;
    clang::AccessSpecifier access;

    Base() = delete;
    Base(const clang::CXXRecordDecl* derived, const clang::CXXBaseSpecifier& base);
};

struct Method
{
    std::string returnType;
    std::string name;
    std::vector<std::string> paramTypes;
    clang::AccessSpecifier access;

    Method() = delete;
    Method(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method);
    std::string getSignature() const;
};

struct Record
{
    std::string name;
    std::vector<Base> bases;
    std::vector<Method> methods;

    Record() = delete;
    Record(const std::string& name): name(name) {}
    Record(const clang::CXXRecordDecl* decl);
    bool operator<(const Record& other) const { return name < other.name; }
};

void Print(const std::vector<Record>& records);

std::vector<Record> Sorted(const std::set<Record>& records);
