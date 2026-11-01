## A quick example

This quick example includes a fairly messy inheritance tree
with virtual inheritance and multiple virtual overrides.

Build the compilation database:

`bear -- make`

And run in the inheritance tree viewer:

`../itree`

Notice how, in the Makefile, the location of your C++ standard library
needs to be specified with `-isystem /usr/lib/clang/23/include`. This
is a hard requirement and platform dependent. Modify it as needed for your machine
and/or platform.
