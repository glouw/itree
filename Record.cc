#include "Record.hh"

#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/QualTypeNames.h"

#include <map>
#include <iostream>

static constexpr auto ResetColor = llvm::raw_string_ostream::RESET;
static constexpr auto FieldColor = llvm::raw_string_ostream::BRIGHT_MAGENTA;
static constexpr auto PublicColor = llvm::raw_string_ostream::BRIGHT_GREEN;
static constexpr auto ProtectedColor = llvm::raw_string_ostream::BRIGHT_YELLOW;
static constexpr auto PrivateColor = llvm::raw_string_ostream::BRIGHT_RED;
static constexpr auto DefaultColor = llvm::raw_string_ostream::WHITE;
static constexpr auto ImplicitColor = llvm::raw_string_ostream::BRIGHT_BLACK;
static constexpr auto VirtualColor = llvm::raw_string_ostream::BRIGHT_CYAN;
static constexpr auto OverrideColor = llvm::raw_string_ostream::BRIGHT_CYAN;

struct Child
{
    const Record* record;
    clang::AccessSpecifier access;
    bool isVirtual;
    Child(const Record* record, clang::AccessSpecifier access, bool isVirtual): record(record), access(access), isVirtual(isVirtual) {}

    std::string getSignature() const
    {
        std::string signature;
        if(isVirtual)
        {
            signature += "virtual";
            signature += " ";
        }
        signature += record->name;
        return signature;
    }
};

static std::string GetTypeName(const clang::CXXRecordDecl* derived, const clang::QualType type)
{
    const clang::ASTContext& context = derived->getASTContext();
    clang::PrintingPolicy policy(context.getLangOpts());
    return clang::TypeName::getFullyQualifiedName(type, context, policy);
}

static clang::AccessSpecifier WrapAccessSpecifier(const clang::AccessSpecifier access, const clang::CXXRecordDecl* derived)
{
    if(access == clang::AS_none)
    {
        return derived->isClass() ? clang::AS_private : clang::AS_public;
    }
    return access;
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

std::string Method::getSignature() const
{
    std::string signature;
    if(isVirtual)
    {
        signature += "virtual";
        signature += " ";
    }
    if(isConstructor or isDestructor)
    {
    }
    else
    {
        signature += returnType;
        signature += " ";
    }
    signature += name;
    signature += "(";
    size_t index = 0;
    size_t last = types.size() - 1;
    for(const std::string& type: types)
    {
        signature += type;
        const std::string fieldName = names[index];
        if(not fieldName.empty())
        {
            signature += " ";
            signature += names[index];
        }
        if(index != last)
        {
            signature += ",";
            signature += " ";
        }
        index++;
    }
    signature += ")";
    if(isPureVirtual)
    {
        signature += " ";
        signature += "= 0";
    }
    if(isOverride)
    {
        signature += " ";
        signature += "override";
    }
    if(isMultipleOverride)
    {
        signature += " ";
        signature += "override!";
    }
    signature += ";";
    return signature;
}

std::string Field::getSignature() const
{
    std::string signature;
    signature += type;
    signature += " ";
    signature += name;
    signature += ";";
    return signature;
}

std::string Record::getSignature() const
{
    std::string signature;
    signature += name;
    return signature;
}

Base::Base(const clang::CXXRecordDecl* derived, const clang::CXXBaseSpecifier& base)
{
    type = GetTypeName(derived, base.getType().getCanonicalType());
    access = WrapAccessSpecifier(base.getAccessSpecifier(), derived);
    isVirtual = base.isVirtual();
}

Field::Field(const clang::CXXRecordDecl* derived, const clang::FieldDecl* field)
{
    type = GetTypeName(derived, field->getType().getCanonicalType());
    name = field->getName();
    access = WrapAccessSpecifier(field->getAccess(), derived);
}

Method::Method(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method)
{
    returnType = GetTypeName(derived, method->getReturnType().getCanonicalType());
    for(const clang::ParmVarDecl* param: method->parameters())
    {
        types.push_back(GetTypeName(derived, param->getType().getCanonicalType()));
        names.push_back(param->getName().str());
    }
    name = method->getNameAsString() + ReadTemplateParameters(derived, method);
    access = WrapAccessSpecifier(method->getAccess(), derived);
    isImplicit = method->isImplicit();
    isVirtual = method->isVirtual();
    isPureVirtual = method->isPureVirtual();
    isOverride = method->size_overridden_methods() == 1;
    isMultipleOverride = method->size_overridden_methods() > 1;
    isConstructor = llvm::isa<clang::CXXConstructorDecl>(method);
    isDestructor = llvm::isa<clang::CXXDestructorDecl>(method);
}

Record::Record(const clang::CXXRecordDecl* derived)
{
    name = GetTypeName(derived, derived->getASTContext().getCanonicalTagType(derived));
    for(const clang::CXXBaseSpecifier& base: derived->bases())
    {
        bases.push_back(Base(derived, base));
    }
    for (const clang::FieldDecl* field: derived->fields())
    {
        fields.push_back(Field(derived, field));
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

static llvm::raw_ostream::Colors AccessColor(const clang::AccessSpecifier access)
{
    if(access == clang::AccessSpecifier::AS_public)
    {
        return PublicColor;
    }
    if(access == clang::AccessSpecifier::AS_protected)
    {
        return ProtectedColor;
    }
    if(access == clang::AccessSpecifier::AS_private)
    {
        return PrivateColor;
    }
    return ResetColor;
}

static std::string Colored(const std::string& in, const llvm::raw_ostream::Colors color)
{
    std::string string;
    llvm::raw_string_ostream stream(string);
    stream.enable_colors(true);
    stream.changeColor(color);
    stream << in;
    stream.resetColor();
    stream.flush();
    return string;
}

static void Print(const Record& record, const std::map<std::string, std::vector<Child>>& children, std::set<const Record*>& seen, const std::string& prefix)
{
    const auto it = children.find(record.name);
    const bool canPrint = seen.count(&record) == 0;
    const bool hasChildren = it != children.end();
    const size_t memberTotal = canPrint ? (record.fields.size() + record.methods.size()) : 0;
    const size_t childTotal = hasChildren ? it->second.size() : 0;
    const size_t total = memberTotal + childTotal;
    size_t index = 0;
    if(canPrint)
    {
        seen.insert(&record);
        for(const Field& field: record.fields)
        {
            const bool last = index == total - 1;
            const std::string left = last ? "└── " : "├── ";
            llvm::outs()
                << Colored(prefix, FieldColor)
                << Colored(left, AccessColor(field.access))
                << Colored(field.getSignature(), FieldColor)
                << '\n';
            index++;
        }
        for(const Method& method: record.methods)
        {
            const bool last = index == total - 1;
            const std::string left = last ? "└── " : "├── ";
            llvm::outs()
                << Colored(prefix, DefaultColor)
                << Colored(left, AccessColor(method.access))
                << Colored(method.getSignature(),
                        method.isImplicit ? ImplicitColor :
                        method.isPureVirtual ? VirtualColor:
                        method.isVirtual ? VirtualColor :
                        method.isOverride ? OverrideColor :
                        DefaultColor)
                << '\n';
            index++;
        }
    }
    if(hasChildren)
    {
        const auto& kids = it->second;
        for(const Child& child: kids)
        {
            const bool last = index == total - 1;
            const std::string left = last ? "└── " : "├── ";
            const std::string rite = last ? "    " : "│   ";
            llvm::outs()
                << Colored(prefix, DefaultColor)
                << Colored(left, AccessColor(child.access))
                << Colored(child.getSignature(), DefaultColor)
                << '\n';
            Print(*child.record, children, seen, prefix + rite);
            index++;
        }
    }
}

static void Print(const Record& record, const std::map<std::string, std::vector<Child>>& children)
{
    llvm::outs() << Colored(record.getSignature(), DefaultColor) << '\n';
    std::set<const Record*> seen;
    Print(record, children, seen, "");
}

void Print(const std::vector<Record>& records)
{
    std::map<std::string, std::vector<Child>> children;
    for(const Record& record: records)
    {
        for(const Base& base: record.bases)
        {
            const Child child(&record, base.access, base.isVirtual);
            children[base.type].push_back(child);
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
            if(auto it = records.find(Record(base.type)); it != records.end())
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
