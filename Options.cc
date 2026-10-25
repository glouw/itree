#include "Options.hh"

#include <string>
#include <iostream>

namespace Options
{
    void Collect(int argc, const char* argv[])
    {
        for(int i = 0; i < argc; i++)
        {
            const std::string argument = argv[i];
            if(argument == "--implicit")
            {
                CollectImplicits = true;
            }
            if(argument == "--help")
            {
                std::cerr
                    << "--implicit: display implicitly generated member functions such constructors, destructors, and operators"
                    << std::endl;
                std::cerr
                    << "--help: you are already here!"
                    << std::endl;
                std::exit(0);
            }
        }
    }
}
