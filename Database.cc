#include "Database.hh"

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <iostream>

namespace Database
{
    struct Database
    {
        std::unique_ptr<clang::tooling::CompilationDatabase> handle;
        const std::string path = ".";
        const std::string noDatabaseFound =
            "compilation_database.json not found\n"
            "Do you use make? Generate one with `bear -- make`,\n"
            "Do you use CMake? Use `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`\n"
            "Then, in either or, be sure to specify your std:: system include path:\n"
            "\teg: -isystem /path/to/your/c++/std/library"
        ;

        Database()
        {
            std::string error;
            handle = clang::tooling::JSONCompilationDatabase::loadFromDirectory(path, error);
            if(not handle)
            {
                std::cerr << noDatabaseFound << std::endl;
                std::exit(1);
            }
        }

        clang::tooling::ClangTool access()
        {
            return clang::tooling::ClangTool(*handle, handle->getAllFiles());
        }
    };

    Database& Instance()
    {
        static Database database;
        return database;
    }

    clang::tooling::CompilationDatabase& Handle()
    {
        return *Instance().handle;
    }
}
