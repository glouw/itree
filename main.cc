#include "RecordVisitor.hh"
#include "Options.hh"

int main(int argc, const char* argv[])
{
    Options::Collect(argc, argv);
    VisitRecords();
}
