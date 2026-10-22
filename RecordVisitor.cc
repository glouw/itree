#include "RecordVisitor.hh"

#include "Database.hh"
#include "Record.hh"

#include <clang/AST/RecursiveASTVisitor.h>

namespace RecordVisitor
{
struct Visitor: clang::RecursiveASTVisitor<Visitor>
{
    std::set<Record::Record>& records;

    Visitor(std::set<Record::Record>& records): records(records) {}

    bool shouldVisitTemplateInstantiations() const
    {
        return true;
    }

    bool VisitCXXRecordDecl(clang::CXXRecordDecl* decl)
    {
        if(decl->getASTContext().getSourceManager().isInSystemHeader(decl->getLocation()))
        {
            return true;
        }
        if(not decl->isThisDeclarationADefinition())
        {
            return true;
        }
        if(decl->isImplicit())
        {
            return true;
        }
        if(not decl->getIdentifier())
        {
            return true;
        }
        if(decl->getDescribedClassTemplate())
        {
            return true;
        }
        if(auto* spec = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl))
        {
            const auto kind = spec->getSpecializationKind();
            if(kind == clang::TSK_Undeclared or kind == clang::TSK_ExplicitInstantiationDeclaration)
            {
                return true;
            }
        }
        records.insert(Record::Record(decl));
        return true;
    }
};

struct Consumer: clang::ASTConsumer
{
    Visitor visitor;

    Consumer(std::set<Record::Record>& records): visitor(records) {}

    void HandleTranslationUnit(clang::ASTContext& context) override
    {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
};

struct Action: clang::ASTFrontendAction
{
    std::set<Record::Record>& records;

    Action(std::set<Record::Record>& records): records(records) {}

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef) override
    {
        return std::make_unique<Consumer>(records);
    }
};

struct ActionFactory: clang::tooling::FrontendActionFactory
{
    std::set<Record::Record>& records;

    ActionFactory(std::set<Record::Record>& records): records(records) {}

    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<Action>(records);
    }
};

void Visit()
{
    std::set<Record::Record> records;
    ActionFactory factory(records);
    clang::tooling::ClangTool tool = Database::Access();
    if(tool.run(&factory) == 0)
    {
        Print(Sorted(records));
    }
}
}
