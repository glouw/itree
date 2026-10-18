#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Tooling/JSONCompilationDatabase.h>

namespace llmv
{

enum class AccessSpec
{
    Private, Protected, Public
};

struct Base
{
    std::string name;
    AccessSpec spec;
};

std::string MethodToString(const clang::CXXMethodDecl* method)
{
    std::string signature;
    if(method->isVirtual())
    {
        signature += "virtual";
        signature += " ";
    }
    if(not llvm::isa<clang::CXXConstructorDecl>(method))
    {
        if(not llvm::isa<clang::CXXDestructorDecl>(method))
        {
            signature += method->getReturnType().getAsString();
            signature += " ";
        }
    }
    signature += method->getNameAsString();
    signature += "(";
    for(unsigned i = 0; i < method->getNumParams(); i++)
    {
        const auto* param = method->getParamDecl(i);
        if(i > 0)
        {
            signature += ",";
            signature += " ";
        }
        signature += param->getType().getAsString();
        if(not param->getName().empty())
        {
            signature += " ";
            signature += param->getNameAsString();
        }
    }
    signature += ")";
    if(method->isConst())
    {
        signature += " ";
        signature += "const";
    }
    return signature;
}

struct Record
{
    std::string name;
    std::vector<Base> bases;
    std::vector<std::string> methods;
    bool isClass = false;

    Record() = default;
    Record(const std::string& name): name(name) {}
    Record(const clang::CXXRecordDecl* decl): name(decl->getNameAsString()), isClass(decl->isClass())
    {
        for(const auto& base: decl->bases())
        {
            const std::string baseName = base.getType().getAsString();
            AccessSpec spec;
            switch(base.getAccessSpecifier())
            {
            case clang::AS_public:
                spec = AccessSpec::Public;
                break;
            case clang::AS_protected:
                spec = AccessSpec::Protected;
                break;
            case clang::AS_private:
                spec = AccessSpec::Private;
                break;
            case clang::AS_none:
                spec = isClass ? AccessSpec::Private : AccessSpec::Public;
                break;
            }
            bases.push_back(Base(baseName, spec));
        }
        for(const auto method: decl->methods())
        {
            if(not method->isImplicit())
            {
                methods.push_back(MethodToString(method));
            }
        }
    }

    bool operator<(const Record& other) const
    {
        return name < other.name;
    }
};

std::vector<Record> TopologicalSort(const std::set<Record>& records)
{
    std::vector<Record> result;
    std::set<std::string> visited;
    std::set<std::string> active;
    std::function<void(const Record&)> visit = [&](const Record& record)
    {
        if(active.count(record.name))
        {
            const std::string error = "graph cycle detected: " + record.name;
            llvm::report_fatal_error(llvm::Twine(error));
        }
        if(not visited.insert(record.name).second)
        {
            return;
        }
        active.insert(record.name);
        for(const auto& base: record.bases)
        {
            auto it = records.find(Record(base.name));
            if(it != records.end())
            {
                visit(*it);
            }
        }
        active.erase(record.name);
        result.push_back(record);
    };
    for(const auto& record: records)
    {
        visit(record);
    }
    return result;
}

void Print(
    const std::map<std::string, std::vector<std::string>>& methods,
    const std::map<std::string, std::vector<std::string>>& bases,
    const std::string& name,
    const std::string& head,
    const std::string& tail)
{
    static const std::vector<std::string> empty;
    llvm::outs() << head << name << '\n';
    const auto f = methods.find(name);
    const auto c = bases.find(name);
    const auto& function = f == methods.end() ? empty : f->second;
    const auto& child = c == bases.end() ? empty : c->second;
    const size_t total = function.size() + child.size();
    for(size_t i = 0; i < total; i++)
    {
        const bool last = i + 1 == total;
        if(i < function.size())
        {
            const std::string left = tail + (last ? "└── " : "├── ");
            const std::string rite = tail + (last ? "    " : "│   ");
            llvm::outs() << left << function[i] << "\n";
        }
        else
        {
            const std::string color = "\033[35m";
            const std::string reset = "\033[0m";
            const std::string left = tail + (last ? color + "└── " + reset : color + "├── " + reset); // COLOR CODE
            const std::string rite = tail + (last ? "    " : "│   ");
            const std::string name = child[i - function.size()];
            Print(methods, bases, name, left, rite);
        }
    }
}

void Print(const std::vector<Record>& records)
{
    std::set<std::string> known;
    std::map<std::string, std::vector<std::string>> methods;
    for(const auto& record: records)
    {
        known.insert(record.name);
        methods[record.name] = record.methods;
    }
    std::set<std::string> roots;
    std::map<std::string, std::vector<std::string>> bases;
    for(const auto& record: records)
    {
        if(record.bases.empty())
        {
            roots.insert(record.name);
        }
        for(const auto& base : record.bases)
        {
            bases[base.name].push_back(record.name);
            if(not known.count(base.name))
            {
                roots.insert(base.name);
            }
        }
    }
    for(const auto& root : roots)
    {
        Print(methods, bases, root, "", "");
    }
}

struct RecordVisitor: clang::RecursiveASTVisitor<RecordVisitor>
{
    std::set<Record>& records;

    RecordVisitor(std::set<Record>& records): records(records) {}

    bool VisitCXXRecordDecl(clang::CXXRecordDecl* decl)
    {
        const bool isDefinition = decl->isThisDeclarationADefinition();
        const bool isIdentifier = decl->getIdentifier();
        const bool isUserHeader = not decl->getASTContext().getSourceManager().isInSystemHeader(decl->getLocation());
        const bool isExplicit = not decl->isImplicit();
        if(isDefinition and isExplicit and isUserHeader and isExplicit)
        {
            Record record = Record(decl);
            records.insert(record);
        }
        return true;
    }
};

struct RecordConsumer: clang::ASTConsumer
{
    RecordVisitor visitor;

    RecordConsumer(std::set<Record>& records): visitor(records) {}

    void HandleTranslationUnit(clang::ASTContext& context) override
    {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
};

struct RecordAction: clang::ASTFrontendAction
{
    std::set<Record>& records;

    RecordAction(std::set<Record>& records): records(records) {}

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& compiler, clang::StringRef file) override
    {
        return std::make_unique<RecordConsumer>(records);
    }
};

struct RecordActionFactory: clang::tooling::FrontendActionFactory
{
    std::set<Record>& records;

    RecordActionFactory(std::set<Record>& records): records(records) {}

    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<RecordAction>(records);
    }
};

void Run()
{
    std::string error;
    auto database = clang::tooling::JSONCompilationDatabase::loadFromDirectory(".", error);
    if(not database)
    {
        llvm::report_fatal_error(llvm::Twine(error));
    }
    clang::tooling::ClangTool tool(*database, database->getAllFiles());
    std::set<Record> records;
    RecordActionFactory factory(records);
    if(tool.run(&factory) == 0)
    {
        const std::vector<Record> sorted = TopologicalSort(records);
        Print(sorted);
    }
}

}

int main()
{
    llmv::Run();
}
