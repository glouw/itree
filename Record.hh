#pragma once

#include "Tokens.hh"
#include "clang/AST/DeclCXX.h"

#include <string>
#include <set>
#include <vector>

namespace Record
{
    using Color = llvm::raw_string_ostream::Colors;

    namespace Colors
    {
        constexpr Color Bound = Color::BRIGHT_BLACK;
        constexpr Color Explicit = Color::WHITE;
        constexpr Color Field = Color::MAGENTA;
        constexpr Color Implicit = Color::BRIGHT_BLACK;
        constexpr Color Child = Color::BRIGHT_WHITE;
        constexpr Color Record = Color::BRIGHT_WHITE;
        constexpr Color Override = Color::BRIGHT_CYAN;
        constexpr Color Private = Color::BRIGHT_RED;
        constexpr Color Protected = Color::BRIGHT_BLUE;
        constexpr Color Public = Color::BRIGHT_GREEN;
        constexpr Color PureVirtual = Color::CYAN;
        constexpr Color Reset = Color::RESET;
        constexpr Color Virtual = Color::BRIGHT_CYAN;
        constexpr Color Highlight = Color::BRIGHT_YELLOW;
    };

    struct Access
    {
        clang::AccessSpecifier access;

        Color color() const;
        std::string str() const;
        clang::AccessSpecifier inherit(const clang::CXXRecordDecl* derived);
    };

    struct Loc
    {
        std::string path = "null";
        unsigned line = 0;

        Loc() = default;
        Loc(const clang::CXXRecordDecl* derived);
    };

    struct Base
    {
        std::string type;
        bool isVirtual;
        Loc loc;
        Access access;

        Base() = delete;
        Base(const clang::CXXRecordDecl* derived, const clang::CXXBaseSpecifier& base);
    };

    struct Field
    {
        std::string type;
        std::string name;
        Access access;

        Field() = delete;
        Field(const clang::CXXRecordDecl* derived, const clang::FieldDecl* field);
        Color color() const;
        std::string signature() const;
    };

    struct Method
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
        Access access;

        Method() = delete;
        Method(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method);
        Color color() const;
        std::string signature() const;
    };

    struct Record
    {
        std::string name;
        std::string alias;
        std::vector<Base> bases;
        std::vector<Field> fields;
        std::vector<Method> methods;
        bool isClass;
        Loc loc;
        Access access;

        Record() = delete;
        Record(const std::string& name): name(name) {}
        Record(const clang::CXXRecordDecl* decl);
        bool operator<(const Record& other) const { return name < other.name; }
        Color color() const;
        std::string signature() const;
    };

    struct Child
    {
        std::string name;
        const Record* record;
        bool isVirtual;
        Loc loc;
        Access access;

        Child() = delete;
        Child(const Record* record, const Base& base);
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
        std::map<std::string, std::set<const Record*>> belongs;

        Sorter() = delete;
        Sorter(const std::set<Record>& records): records(records) {}
        void sort(const Record& record);
        void sort();
        void collectBelongings(const Record* root, const Record* record);
        bool belongsTo(const std::string& name) const { return belongs.count(name) == 1; }
    };

    struct Printer
    {
        const Sorter& sorter;
        std::set<const Record*> seen;
        std::string buffer;
        llvm::raw_string_ostream stream = llvm::raw_string_ostream(buffer);
        std::string highlight;

        Printer() = delete;
        Printer(const Sorter& sorter, const std::string highlight = "", const bool withColor = true);
        template<typename T>
        void print(const T& value, const std::string& prefix, const std::string& rite);
        void print(const Record& record, const std::string& prefix);
        void print(const Record& record);
        void flush();
    };
}
