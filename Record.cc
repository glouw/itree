#include "Record.hh"

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/QualTypeNames.h"

#include <map>
#include <iostream>

struct Child
{
    const Record* record;
    clang::AccessSpecifier access;
    Child(const Record* record, clang::AccessSpecifier access): record(record), access(access) {}
};

static std::string GetTypeName(const clang::CXXRecordDecl* derived, const clang::QualType type)
{
    const clang::ASTContext& context = derived->getASTContext();
    clang::PrintingPolicy policy(context.getLangOpts());
    return clang::TypeName::getFullyQualifiedName(type, context, policy);
}

Base::Base(const clang::CXXRecordDecl* derived, const clang::CXXBaseSpecifier& base)
{
    name = GetTypeName(derived, base.getType().getCanonicalType());
    access = base.getAccessSpecifier();
    if(access == clang::AS_none)
    {
        access = derived->isClass() ? clang::AS_private : clang::AS_public;
    }
}

static std::string ReadTemplateParameters(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method)
{
    const clang::ASTContext& context = derived->getASTContext();
    clang::PrintingPolicy policy(context.getLangOpts());
    std::string parameters;
    if(const clang::TemplateArgumentList* list = method->getTemplateSpecializationArgs())
    {
        parameters += "<";
        const auto& args = list->asArray();
        size_t last = args.size() - 1;
        size_t index = 0;
        for(const clang::TemplateArgument& arg: args)
        {
            std::string string;
            llvm::raw_string_ostream stream(string);
            arg.print(policy, stream, true);
            parameters += stream.str();
            if(index != last)
            {
                parameters += ",";
            }
            index++;
        }
        parameters += ">";
    }
    return parameters;
}

Method::Method(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method)
{
    returnType = GetTypeName(derived, method->getReturnType().getCanonicalType());
    for(const clang::ParmVarDecl* param: method->parameters())
    {
        paramTypes.push_back(GetTypeName(derived, param->getType().getCanonicalType()));
    }
    name = method->getNameAsString();
    name += ReadTemplateParameters(derived, method);
    access = method->getAccess();
    if(access == clang::AS_none)
    {
        access = derived->isClass() ? clang::AS_private : clang::AS_public;
    }
}

std::string Method::getSignature() const
{
    std::string signature;
    signature += returnType;
    signature += " ";
    signature += name;
    signature += "(";
    size_t index = 0;
    size_t last = paramTypes.size() - 1;
    for(const std::string& paramType : paramTypes)
    {
        signature += paramType;
        if(index not_eq last)
        {
            signature += ",";
            signature += " ";
        }
        index++;
    }
    signature += ")";
    return signature;
}

Record::Record(const clang::CXXRecordDecl* derived)
{
    name = GetTypeName(derived, derived->getASTContext().getCanonicalTagType(derived));
    for(const clang::CXXBaseSpecifier& base: derived->bases())
    {
        bases.push_back(Base(derived, base));
    }
    for(const clang::Decl* decl: derived->decls())
    {
        if(const clang::CXXMethodDecl* method = llvm::dyn_cast<clang::CXXMethodDecl>(decl))
        {
            methods.push_back(Method(derived, method));
        }
        else
        if(const clang::FunctionTemplateDecl* functionTemplate = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl))
        {
            for(const clang::FunctionDecl* functionDecl: functionTemplate->specializations())
            {
                if(const clang::CXXMethodDecl* templatedMethod = llvm::dyn_cast<clang::CXXMethodDecl>(functionDecl))
                {
                    methods.emplace_back(derived, templatedMethod);
                }
            }
        }
    }
}

static llvm::raw_ostream::Colors Color(const clang::AccessSpecifier access)
{
    if(access == clang::AccessSpecifier::AS_public)    return llvm::raw_ostream::Colors::BRIGHT_GREEN;
    if(access == clang::AccessSpecifier::AS_protected) return llvm::raw_ostream::Colors::BRIGHT_YELLOW;
    if(access == clang::AccessSpecifier::AS_private)   return llvm::raw_ostream::Colors::BRIGHT_RED;
    return llvm::raw_ostream::Colors::RESET;
}

static std::string ColoredString(const std::string& in, const llvm::raw_ostream::Colors color)
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

static void Print(const Record& record, const std::map<std::string, std::vector<Child>>& children, const std::string& prefix)
{
    const auto it = children.find(record.name);
    const bool hasChildren = it != children.end();
    const size_t total = record.methods.size() + (hasChildren ? it->second.size() : 0);
    size_t index = 0;
    for(const Method& method: record.methods)
    {
        const bool last = index == total - 1;
        const std::string left = last ? "└── " : "├── ";
        llvm::outs() << prefix << ColoredString(left, Color(method.access)) << method.getSignature() << '\n';
        index++;
    }
    if(hasChildren)
    {
        const auto& kids = it->second;
        for(const Child& child: kids)
        {
            const bool last = index == total - 1;
            const std::string left = last ? "└── " : "├── ";
            const std::string rite = last ? "    " : "│   ";
            llvm::outs() << prefix << ColoredString(left, Color(child.access)) << child.record->name << '\n';
            Print(*child.record, children, prefix + rite);
            index++;
        }
    }
}

static void Print(const Record& record, const std::map<std::string, std::vector<Child>>& children)
{
    llvm::outs() << record.name << '\n';
    Print(record, children, "");
}

void Print(const std::vector<Record>& records)
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

static void Sort(const std::set<Record>& records, const Record& record, std::set<std::string>& visited, std::vector<Record>& result)
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

std::vector<Record> Sorted(const std::set<Record>& records)
{
    std::vector<Record> result;
    std::set<std::string> visited;
    for(const Record& record: records)
    {
        Sort(records, record, visited, result);
    }
    return result;
}
