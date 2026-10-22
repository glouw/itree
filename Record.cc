#include "Record.hh"
#include "Options.hh"

#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/QualTypeNames.h>
#include <map>
#include <iostream>

namespace Record
{
namespace Colors
{
    static constexpr Color Bound = Color::BRIGHT_BLACK;
    static constexpr Color Explicit = Color::WHITE;
    static constexpr Color Field = Color::MAGENTA;
    static constexpr Color Implicit = Color::BRIGHT_BLACK;
    static constexpr Color Child = Color::BRIGHT_WHITE;
    static constexpr Color Record = Color::BRIGHT_WHITE;
    static constexpr Color Override = Color::BRIGHT_CYAN;
    static constexpr Color Private = Color::BRIGHT_RED;
    static constexpr Color Protected = Color::BRIGHT_BLUE;
    static constexpr Color Public = Color::BRIGHT_GREEN;
    static constexpr Color PureVirtual = Color::CYAN;
    static constexpr Color Reset = Color::RESET;
    static constexpr Color Virtual = Color::BRIGHT_CYAN;
    static constexpr Color Highlight = Color::BRIGHT_YELLOW;
};

namespace Tokens
{
    static const std::string Bends = "└── ";
    static const std::string Bound = "│   ";
    static const std::string Class = "class";
    static const std::string Comma = ",";
    static const std::string Empty = "";
    static const std::string Explicit = "explicit";
    static const std::string Implicit = "implicit";
    static const std::string Junct = "├── ";
    static const std::string LeftAngleBracket = "<";
    static const std::string LeftRoundBracket = "(";
    static const std::string MultipleOverride = "multiple override";
    static const std::string Newline = "\n";
    static const std::string OfPureVirtual = "= 0";
    static const std::string Override = "override";
    static const std::string Private = "private";
    static const std::string Protected = "protected";
    static const std::string Public = "public";
    static const std::string RiteAngleBracket = ">";
    static const std::string RiteRoundBracket = ")";
    static const std::string Semicolon = ";";
    static const std::string Space = " ";
    static const std::string Struct = "struct";
    static const std::string Tab = "    ";
    static const std::string Virtual = "virtual";
    static const std::string ExtendsWith = "extends with";
};

Color Access::color() const
{
    switch(access)
    {
    case clang::AccessSpecifier::AS_public:
        return Colors::Public;
    case clang::AccessSpecifier::AS_protected:
        return Colors::Protected;
    case clang::AccessSpecifier::AS_private:
        return Colors::Private;
    default:
        return Colors::Reset;
    }
}

Color Method::color() const
{
    return isImplicit ? Colors::Implicit
         : isOverride or isMultipleOverride ? Colors::Override
         : isPureVirtual ? Colors::PureVirtual
         : isVirtual ? Colors::Virtual
         : Colors::Explicit;
}

Color Record::color() const
{
    return Colors::Record;
}

Color Field::color() const
{
    return Colors::Field;
}

Color Child::color() const
{
    return Colors::Child;
}

static std::string TypeName(const clang::CXXRecordDecl* derived, const clang::QualType type)
{
    const clang::ASTContext& context = derived->getASTContext();
    clang::PrintingPolicy policy(context.getLangOpts());
    return clang::TypeName::getFullyQualifiedName(type, context, policy);
}

static std::string ReadTemplateParameters(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method)
{
    const clang::ASTContext& context = derived->getASTContext();
    clang::PrintingPolicy policy(context.getLangOpts());
    std::string parameters;
    if(const clang::TemplateArgumentList* list = method->getTemplateSpecializationArgs())
    {
        parameters += Tokens::LeftAngleBracket;
        const auto& args = list->asArray();
        const size_t last = args.size() - 1;
        size_t index = 0;
        for(const clang::TemplateArgument& arg: args)
        {
            std::string string;
            llvm::raw_string_ostream stream(string);
            arg.print(policy, stream, true);
            parameters += stream.str();
            if(index != last)
            {
                parameters += Tokens::Comma;
            }
            index++;
        }
        parameters += Tokens::RiteAngleBracket;
    }
    return parameters;
}

std::string Access::str() const
{
    std::string string;
    if(access == clang::AccessSpecifier::AS_public)
    {
        string = Tokens::Public;
    }
    if(access == clang::AccessSpecifier::AS_protected)
    {
        string = Tokens::Protected;
    }
    if(access == clang::AccessSpecifier::AS_private)
    {
        string = Tokens::Private;
    }
    return string;
}

std::string Child::signature() const
{
    std::string signature;
    signature += Tokens::ExtendsWith;
    signature += Tokens::Space;
    signature += Access::str();
    signature += Tokens::Space;
    if(isVirtual)
    {
        signature += Tokens::Virtual;
        signature += Tokens::Space;
    }
    signature += record->name;
    return signature;
}

std::string Method::signature() const
{
    std::string signature;
    if(isImplicit)
    {
        signature += Tokens::Implicit;
        signature += Tokens::Space;
    }
    signature += Access::str();
    signature += Tokens::Space;
    if(isVirtual)
    {
        signature += Tokens::Virtual;
        signature += Tokens::Space;
    }
    if(isConstructor or isDestructor)
    {
    }
    else
    {
        signature += returnType;
        signature += Tokens::Space;
    }
    signature += name;
    signature += Tokens::LeftRoundBracket;
    size_t index = 0;
    size_t last = types.size() - 1;
    for(const std::string& type: types)
    {
        signature += type;
        const std::string fieldName = names[index];
        if(not fieldName.empty())
        {
            signature += Tokens::Space;
            signature += names[index];
        }
        if(index != last)
        {
            signature += Tokens::Comma;
            signature += Tokens::Space;
        }
        index++;
    }
    signature += Tokens::RiteRoundBracket;
    if(isPureVirtual)
    {
        signature += Tokens::Space;
        signature += Tokens::OfPureVirtual;
    }
    if(isOverride)
    {
        signature += Tokens::Space;
        signature += Tokens::Override;
    }
    if(isMultipleOverride)
    {
        signature += Tokens::Space;
        signature += Tokens::MultipleOverride;
    }
    signature += Tokens::Semicolon;
    return signature;
}

std::string Field::signature() const
{
    std::string signature;
    signature += Access::str();
    signature += Tokens::Space;
    signature += type;
    signature += Tokens::Space;
    signature += name;
    signature += Tokens::Semicolon;
    return signature;
}

std::string Record::signature() const
{
    std::string signature;
    signature += isClass ? Tokens::Class : Tokens::Struct;
    signature += Tokens::Space;
    signature += name;
    return signature;
}

clang::AccessSpecifier Access::inherit(const clang::CXXRecordDecl* derived)
{
    if(access == clang::AS_none)
    {
        return derived->isClass() ? clang::AS_private : clang::AS_public;
    }
    return access;
}

Child::Child(const Record* record, clang::AccessSpecifier access, const bool isVirtual)
{
    this->name = record->name;
    this->record = record;
    this->access = access;
    this->isVirtual = isVirtual;
}

Field::Field(const clang::CXXRecordDecl* derived, const clang::FieldDecl* field)
{
    type = TypeName(derived, field->getType());
    name = field->getName();
    access = field->getAccess();
    Access::inherit(derived);
}

Method::Method(const clang::CXXRecordDecl* derived, const clang::CXXMethodDecl* method)
{
    returnType = TypeName(derived, method->getReturnType());
    for(const clang::ParmVarDecl* param: method->parameters())
    {
        types.push_back(TypeName(derived, param->getType()));
        names.push_back(param->getName().str());
    }
    name = method->getNameAsString() + ReadTemplateParameters(derived, method);
    access = method->getAccess();
    Access::inherit(derived);
    isImplicit = method->isImplicit();
    isVirtual = method->isVirtual();
    isPureVirtual = method->isPureVirtual();
    isOverride = method->size_overridden_methods() == 1;
    isMultipleOverride = method->size_overridden_methods() > 1;
    isConstructor = llvm::isa<clang::CXXConstructorDecl>(method);
    isDestructor = llvm::isa<clang::CXXDestructorDecl>(method);
}

Base::Base(const clang::CXXRecordDecl* derived, const clang::CXXBaseSpecifier& base)
{
    type = TypeName(derived, base.getType().getCanonicalType());
    access = base.getAccessSpecifier();
    Access::inherit(derived);
    isVirtual = base.isVirtual();
}

Record::Record(const clang::CXXRecordDecl* derived)
{
    name = TypeName(derived, derived->getASTContext().getCanonicalTagType(derived));
    isClass = derived->isClass();
    access = isClass ? clang::AS_private : clang::AS_public;
    for(const clang::CXXBaseSpecifier& base: derived->bases())
    {
        bases.push_back(Base(derived, base));
    }
    for(const clang::FieldDecl* field: derived->fields())
    {
        fields.push_back(Field(derived, field));
    }
    for(const clang::Decl* decl: derived->decls())
    {
        if(const clang::CXXMethodDecl* method = llvm::dyn_cast<clang::CXXMethodDecl>(decl))
        {
            const Method self = Method(derived, method);
            if(self.isImplicit and not Options::CollectImplicits)
            {
                continue;
            }
            methods.push_back(self);
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

void Sorter::sort(const Record& record)
{
    if(visited.insert(record.name).second)
    {
        for(const Base& base: record.bases)
        {
            if(auto iterator = records.find(Record(base.type)); iterator != records.end())
            {
                sort(*iterator);
            }
        }
        result.push_back(record);
    }
}

void Sorter::collectRootsTo(const Record* root, const Record* record)
{
    rootsTo[record->name].insert(root);
    const auto iterator = children.find(record->name);
    const bool hasChildren = iterator != children.end();
    if(hasChildren)
    {
        const std::vector<Child>& kids = iterator->second;
        for(const Child& child: kids)
        {
            collectRootsTo(root, child.record);
        }
    }
}

void Sorter::sort()
{
    for(const Record& record: records)
    {
        sort(record);
    }
    for(const Record& record: result)
    {
        for(const Base& base: record.bases)
        {
            const Child child(&record, base.access, base.isVirtual);
            children[base.type].push_back(child);
        }
    }
    for(const Record& record: result)
    {
        if(record.bases.empty())
        {
            roots.push_back(&record);
        }
    }
    for(const Record* root: roots)
    {
        collectRootsTo(root, root);
    }
}

struct Printer
{
    const Sorter& sorter;
    std::set<const Record*> seen;
    std::string buffer;
    llvm::raw_string_ostream stream = llvm::raw_string_ostream(buffer);
    std::string highlight;

    Printer(const Sorter& sorter, const std::string highlight = "", const bool withColor = true): sorter(sorter), highlight(highlight)
    {
        stream.enable_colors(withColor);
    }

    void flush()
    {
        stream.flush();
        llvm::outs() << buffer << Tokens::Newline;
        buffer.clear();
    }

    template <typename T>
    void print(const T& value, const std::string& prefix, const std::string& rite)
    {
        const Color color = value.name == highlight ? Colors::Highlight : value.color();
        stream
            << Colors::Bound
            << prefix
            << value.Access::color()
            << rite
            << color
            << value.signature()
            << Colors::Reset
            << Tokens::Newline;
    }

    void print(const Record& record, const std::string& prefix)
    {
        const auto iterator = sorter.children.find(record.name);
        const bool hasChildren = iterator != sorter.children.end();
        const bool canPrint = seen.count(&record) == 0;
        const size_t memberTotal = canPrint ? (record.fields.size() + record.methods.size()) : 0;
        const size_t childTotal = hasChildren ? iterator->second.size() : 0;
        const size_t total = memberTotal + childTotal;
        size_t index = 0;
        const auto last = [&index, &total]() { return index == total - 1; };
        if(canPrint)
        {
            seen.insert(&record);
            for(const auto& value: record.fields)
            {
                print(value, prefix, last() ? Tokens::Bends : Tokens::Junct);
                index++;
            }
            for(const auto& value: record.methods)
            {
                print(value, prefix, last() ? Tokens::Bends : Tokens::Junct);
                index++;
            }
        }
        if(hasChildren)
        {
            const std::vector<Child>& children = iterator->second;
            for(const Child& child: children)
            {
                const std::string left = last() ? Tokens::Tab   : Tokens::Bound;
                const std::string rite = last() ? Tokens::Bends : Tokens::Junct;
                print(child, prefix, rite);
                print(*child.record, prefix + left);
                index++;
            }
        }
    }

    void print(const Record& record)
    {
        print(record, Tokens::Space, Tokens::Empty);
        print(record, Tokens::Space);
        flush();
    }
};

void Print(const Sorter& sorter)
{
    Printer printer(sorter);
    for(const Record* root: sorter.roots)
    {
        printer.print(*root);
    }
}

void Print(const Sorter& sorter, const std::string& name)
{
    Printer printer(sorter, name);
    if(sorter.rootsTo.count(name) != 0)
    {
        const std::set<const Record*>& roots = sorter.rootsTo.at(name);
        for(const Record* root: roots)
        {
            printer.print(*root);
        }
    }
}
}
