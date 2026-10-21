#pragma once

#include "clang/AST/DeclCXX.h"

#include <string>
#include <set>
#include <vector>

struct Base
{
    std::string type;
    clang::AccessSpecifier access;
    bool isVirtual;

    Base() = delete;
    Base(const clang::CXXRecordDecl* derived, const clang::CXXBaseSpecifier& base);
};

struct Field
{
    std::string type;
    std::string name;
    clang::AccessSpecifier access;

    Field() = delete;
    Field(const clang::CXXRecordDecl* derived, const clang::FieldDecl* field);
    std::string getSignature() const;
};

struct Method
{
    std::string returnType;
    std::string name;
    std::vector<std::string> types;
    std::vector<std::string> names;
    clang::AccessSpecifier access;
    bool isImplicit;
    bool isVirtual;
    bool isPureVirtual;
    bool isOverride;
    bool isMultipleOverride;
    bool isConstructor;
    bool isDestructor;

    Method() = delete;
    Method(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method);
    std::string getSignature() const;
};

struct Record
{
    std::string name;
    std::vector<Base> bases;
    std::vector<Field> fields;
    std::vector<Method> methods;

    Record() = delete;
    Record(const std::string& name): name(name) {}
    Record(const clang::CXXRecordDecl* decl);
    bool operator<(const Record& other) const { return name < other.name; }
    std::string getSignature() const;
};

void Print(const std::vector<Record>& records);

std::vector<Record> Sorted(const std::set<Record>& records);
