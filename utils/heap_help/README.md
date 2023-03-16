The utility helps to detect incorrect heap usage like memory leaks. It works via
redefinition of the standard functions which are expected to allocate something
on the heap.

**Quick start**: build your code together with `heap_help.c` and run it with the
environment variable `HHREPORT=1`. For example, like this:
`HHREPORT=1 ./my_exe`. When the app exits, if there are any leaks, a report is
printed saying how many leaks you have.

To use it you need to build it as a part of your application. Like
`gcc my_solution.c heap_help.c`. Or build it as a shared library (.so in Linux,
.dylib in Mac) and then build your code with the resulting lib file.

You can also at any moment check the number of not freed allocations using the
function `heaph_get_alloc_count()`. Ideally before your `main()` function
returns this number should be zero.

Shared library build command for Mac:
```
clang -shared -undefined dynamic_lookup -o libheap.dylib heap_help.c
```
