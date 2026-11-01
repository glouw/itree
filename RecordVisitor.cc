#include "RecordVisitor.hh"

#include "Tokens.hh"
#include "Database.hh"
#include "Record.hh"
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Tooling/AllTUsExecution.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>
#include <algorithm>

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
                const clang::TemplateSpecializationKind kind = spec->getSpecializationKind();
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

    struct Librarian
    {
        const std::set<Record::Record>& records;

        std::map<std::string, std::set<std::string>> names;

        Librarian(const std::set<Record::Record>& records): records(records) {}

        void volunteer()
        {
            for(const Record::Record& record: records)
            {
                names[record.alias].insert(record.name);
            }
        }

        bool aliasExists(const std::string alias) const
        {
            return names.count(alias) == 1;
        }

        std::vector<std::string> getNames(const std::string& alias) const
        {
            std::vector<std::string> out;
            if(aliasExists(alias))
            {
                const std::set<std::string>& all = names.at(alias);
                for(const std::string& each: all)
                {
                    out.push_back(each);
                }
            }
            return out;
        }
    };

    struct Terminal
    {
        const Librarian& librarian;
        const Record::Sorter& sorter;
        bool done = false;

        Terminal() = delete;
        Terminal(const Librarian& librarian, const Record::Sorter& sorter): librarian(librarian), sorter(sorter) {}

        struct Line
        {
            char* string = nullptr;
            Line(Line&&) = delete;
            Line(const Line&) = delete;
            Line& operator=(const Line&) = delete;
            Line& operator=(Line&&) = delete;

            Line()
            {
                string = readline(Tokens::Prompt.data());
                if(string)
                {
                    add_history(string);
                }
            }

            ~Line()
            {
                free(string);
            }

            std::string str() const
            {
                return string;
            }

            bool bad() const
            {
                return string == nullptr;
            }
        };

        void parse()
        {
            const Line read;
            if(read.bad())
            {
                llvm::outs() << Tokens::Newline;
                llvm::outs().flush();
                done = true;
                return;
            }
            const std::string line = read.str();
            if(line == Tokens::QuestionMark)
            {
                Record::Printer printer(sorter);
                for(const Record::Record* root: sorter.roots)
                {
                    printer.print(*root);
                }
            }
            else
            if(sorter.belongsTo(line))
            {
                Record::Printer printer(sorter, line);
                const std::set<const Record::Record*>& roots = sorter.belongs.at(line);
                for(const Record::Record* root: roots)
                {
                    printer.print(*root);
                }
            }
            else
            {
                const std::vector<std::string> names = librarian.getNames(line);
                if(not names.empty())
                {
                    for(const std::string& name: names)
                    {
                        llvm::outs() << name << Tokens::Newline;
                    }
                }
            }
        }
    };

    void Visit()
    {
        std::set<Record::Record> records;
        clang::tooling::AllTUsToolExecutor executor(Database::Handle(), 0);
        llvm::Error error = executor.execute(std::make_unique<ActionFactory>(records));
        if(error)
        {
            llvm::errs() << llvm::toString(std::move(error)) << Tokens::Newline;
            return;
        }
        if(records.empty())
        {
            llvm::outs() << "No records?" << Tokens::Newline;
            return;
        }
        Librarian librarian(records);
        librarian.volunteer();
        Record::Sorter sorter(records);
        sorter.sort();
        Terminal terminal(librarian, sorter);
        while(not terminal.done)
        {
            terminal.parse();
        }
    }
}
