The utility helps to detect incorrect heap usage like memory leaks. It works via
redefinition of the standard functions which are expected to allocate something
on the heap.

To use it you need to build it as a part of your application. Just compile the
`.c` file along with your other files. Or build it as a shared library (.so in
Linux, .dylib in Mac and then build your code with the resulting lib file.

Then in the code you need to call `heaph_init()` first line in your `main()`
function.

After that in any moment you can track how many not freed allocations you have
via `heaph_get_alloc_count()`. Ideally before your `main()` function returns
this number should be zero.

Shared library build command for Mac:
```
clang -shared -undefined dynamic_lookup -o libheap.dylib heap_help.c
```
