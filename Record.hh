#pragma once

#include "clang/AST/DeclCXX.h"

#include <string>
#include <set>
#include <vector>

using Color = llvm::raw_string_ostream::Colors;

struct Access
{
    clang::AccessSpecifier access;

    Color color() const;
    std::string str() const;
    clang::AccessSpecifier inherit(const clang::CXXRecordDecl* derived);
};

struct Base: Access
{
    std::string type;
    bool isVirtual;

    Base() = delete;
    Base(const clang::CXXRecordDecl* derived, const clang::CXXBaseSpecifier& base);
};

struct Field: Access
{
    std::string type;
    std::string name;

    Field() = delete;
    Field(const clang::CXXRecordDecl* derived, const clang::FieldDecl* field);
    Color color() const;
    std::string signature() const;
};

struct Method: Access
{
    std::string returnType;
    std::string name;
    std::vector<std::string> types;
    std::vector<std::string> names;
    bool isImplicit;
    bool isVirtual;
    bool isPureVirtual;
    bool isOverride;
    bool isMultipleOverride;
    bool isConstructor;
    bool isDestructor;

    Method() = delete;
    Method(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method);
    Color color() const;
    std::string signature() const;
};

struct Record: Access
{
    std::string name;
    std::vector<Base> bases;
    std::vector<Field> fields;
    std::vector<Method> methods;
    bool isClass;

    Record() = delete;
    Record(const std::string& name): name(name) {}
    Record(const clang::CXXRecordDecl* decl);
    bool operator<(const Record& other) const { return name < other.name; }
    Color color() const;
    std::string signature() const;
};

struct Child: Access
{
    const Record* record;
    bool isVirtual;

    Child(const Record* record, clang::AccessSpecifier access, bool isVirtual);
    Color color() const;
    std::string signature() const;
};

void Print(const std::vector<Record>& records);

std::vector<Record> Sorted(const std::set<Record>& records);
