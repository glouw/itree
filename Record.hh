#pragma once

#include "clang/AST/DeclCXX.h"

#include <string>
#include <set>
#include <vector>

enum class AccessSpec
{
    Private,
    Protected,
    Public
};

struct Base
{
    std::string name;
    AccessSpec access;
    Base() = delete;
    Base(const clang::CXXBaseSpecifier& base, const clang::CXXRecordDecl* derived);
};

struct Record
{
    std::string name;
    std::vector<Base> bases;
    Record() = delete;
    Record(const std::string& name): name(name) {}
    Record(const clang::CXXRecordDecl* decl);
    bool operator<(const Record& other) const { return name < other.name; }
};

void Print(const std::vector<Record>& records);
std::vector<Record> Sort(const std::set<Record>& records);
