#include "Database.hh"

#include <clang/Tooling/JSONCompilationDatabase.h>

struct Database
{
    std::unique_ptr<clang::tooling::CompilationDatabase> handle;
    const std::string path = ".";

    Database()
    {
        std::string error;
        handle = clang::tooling::JSONCompilationDatabase::loadFromDirectory(path, error);
        if(not handle)
        {
            llvm::report_fatal_error(llvm::Twine(error));
        }
    }

    clang::tooling::ClangTool access()
    {
        return clang::tooling::ClangTool(*handle, handle->getAllFiles());
    }
};

clang::tooling::ClangTool AccessDatabase()
{
    static Database database;
    return database.access();
}
