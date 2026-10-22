#pragma once

#include "clang/AST/DeclCXX.h"

#include <string>
#include <set>
#include <vector>

namespace Record
{
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
    std::string name;
    const Record* record;
    bool isVirtual;

    Child() = delete;
    Child(const Record* record, clang::AccessSpecifier access, bool isVirtual);
    Color color() const;
    std::string signature() const;
};

struct Sorter
{
    const std::set<Record>& records;
    std::set<std::string> visited;
    std::vector<Record> result;
    std::map<std::string, std::vector<Child>> children;
    std::vector<const Record*> roots;
    std::map<std::string, std::set<const Record*>> rootsTo;

    Sorter() = delete;
    Sorter(const std::set<Record>& records): records(records) {}
    void sort(const Record& record);
    void sort();
    void collectRootsTo(const Record* root, const Record* record);
};

void Print(const Sorter& sorter);
void Print(const Sorter& sorter, const std::string& name);
}
