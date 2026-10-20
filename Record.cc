#include "Record.hh"

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/QualTypeNames.h"

#include <map>
#include <iostream>

std::string GetTypeName(const clang::CXXRecordDecl* derived, const clang::QualType type)
{
    const clang::ASTContext& context = derived->getASTContext();
    clang::PrintingPolicy policy(context.getLangOpts());
    return clang::TypeName::getFullyQualifiedName(type, context, policy);
}

Base::Base(const clang::CXXBaseSpecifier& base, const clang::CXXRecordDecl* derived)
    : name(GetTypeName(derived, base.getType().getCanonicalType()))
{
    clang::AccessSpecifier spec = base.getAccessSpecifier();
    if(spec == clang::AS_none)
    {
        spec = derived->isClass() ? clang::AS_private : clang::AS_public;
    }
    if(spec == clang::AS_private)
    {
        access = AccessSpec::Private;
    }
    if(spec == clang::AS_protected)
    {
        access = AccessSpec::Protected;
    }
    if(spec == clang::AS_public)
    {
        access = AccessSpec::Public;
    }
}

Record::Record(const clang::CXXRecordDecl* derived)
    : name(GetTypeName(derived, derived->getASTContext().getCanonicalTagType(derived)))
{
    for(const clang::CXXBaseSpecifier& base: derived->bases())
    {
        bases.push_back(Base(base, derived));
    }
}

struct Child
{
    const Record* record;
    AccessSpec spec;
    Child(const Record* record, AccessSpec spec): record(record), spec(spec) {}
};

static llvm::raw_ostream::Colors
Color(const AccessSpec access)
{
    if(access == AccessSpec::Public)
    {
        return llvm::raw_ostream::Colors::BRIGHT_GREEN;
    }
    if(access == AccessSpec::Protected)
    {
        return llvm::raw_ostream::Colors::BRIGHT_BLUE;
    }
    if(access == AccessSpec::Private)
    {
        return llvm::raw_ostream::Colors::BRIGHT_RED;
    }
    return llvm::raw_ostream::Colors::RESET;
}

static std::string
ColoredString(const std::string& in, const llvm::raw_ostream::Colors color)
{
    std::string string;
    llvm::raw_string_ostream stream(string);
    stream.enable_colors(true);
    stream.changeColor(color);
    stream << in;
    stream.flush();
    stream.resetColor();
    return string;
}

static void
Print(const Record& record, const std::map<std::string, std::vector<Child>>& children, const std::string& prefix)
{
    if(children.count(record.name) > 0)
    {
        const std::vector<Child>& kids = children.at(record.name);
        for(size_t i = 0; i < kids.size(); i++)
        {
            const bool last = i + 1 == kids.size();
            const std::string left = last ? "└── " : "├── ";
            const std::string rite = last ? "    " : "│   ";
            const Child child = kids[i];
            llvm::outs() << prefix << ColoredString(left, Color(child.spec)) << child.record->name << '\n';
            Print(*child.record, children, prefix + rite);
        }
    }
}

static void
Print(const Record& record, const std::map<std::string, std::vector<Child>>& children)
{
    llvm::outs() << record.name << '\n';
    Print(record, children, "");
}

void
Print(const std::vector<Record>& records)
{
    std::map<std::string, std::vector<Child>> children;
    for(const Record& record: records)
    {
        for(const Base& base: record.bases)
        {
            const Child child(&record, base.access);
            children[base.name].push_back(child);
        }
    }
    std::vector<const Record*> roots;
    for(const Record& record: records)
    {
        if(record.bases.empty())
        {
            roots.push_back(&record);
        }
    }
    for(const Record* root: roots)
    {
        Print(*root, children);
    }
}

static void
Sort(const std::set<Record>& records, const Record& record, std::set<std::string>& visited, std::vector<Record>& result)
{
    if(visited.insert(record.name).second)
    {
        for(const Base& base: record.bases)
        {
            if(auto it = records.find(Record(base.name)); it != records.end())
            {
                Sort(records, *it, visited, result);
            }
        }
        result.push_back(record);
    }
}

std::vector<Record>
Sort(const std::set<Record>& records)
{
    std::vector<Record> result;
    std::set<std::string> visited;
    for(const Record& record: records)
    {
        Sort(records, record, visited, result);
    }
    return result;
}
