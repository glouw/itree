#include "RecordVisitor.hh"

#include "Database.hh"
#include "Record.hh"

#include <clang/AST/RecursiveASTVisitor.h>

struct RecordVisitor: clang::RecursiveASTVisitor<RecordVisitor>
{
    std::set<Record>& records;

    RecordVisitor(std::set<Record>& records): records(records) {}

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
        records.insert(Record(decl));
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

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance&, clang::StringRef) override
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

void VisitRecords()
{
    std::set<Record> records;
    RecordActionFactory factory(records);
    clang::tooling::ClangTool tool = AccessDatabase();
    if(tool.run(&factory) == 0)
    {
        Print(Sorted(records));
    }
}
