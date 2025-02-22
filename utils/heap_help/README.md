The utility helps to detect incorrect heap usage like memory leaks. It works via
redefinition of the standard functions which are expected to allocate something
on the heap.

**Quick start**: build your code together with `heap_help.c` with compiler flags
`-ldl -rdynamic`. When the app exits, if there are any leaks, a report is
printed saying how many leaks you have, of which sizes, and can show some basic
stacktraces.

You can also at any moment check the number of not freed allocations using the
function `heaph_get_alloc_count()`. Ideally, before your `main()` function
returns, this number should be zero. Keep in mind that before the process is
terminated, this number might be growing even when you don't allocate anything
due to internal allocations done by the standard library. Those ones are
filtered out at the process exit time.

There are modes which allow to get more or less info:

* `./my_app` - run your app with the default heap help mode;

To enable / disable backtrace collection. On some platforms it might crash, so
try disabling the backtrace if it is failing:

* `HHBACKTRACE=on` - enable backtrace collection and resolution, default.

* `HHBACKTRACE=off` - disable it.

The report mode can help you see how many allocations you do, and some other
reporting details:

* `HHREPORT=l ./my_app` - l = "leaks", the default mode. If there are no leaks,
  then nothing is printed at exit. Otherwise the leaks are printed to stdout
  (not all of them if there are too many) with their sizes and stack traces;

* `HHREPORT=q ./my_app` - q = "quiet", nothing is printed.

* `HHREPORT=v ./my_app` - v = "verbose", either the leaks are printed like with
  the mode "l", or is printed a message saying that "there are no leaks". The
  mode helps to check if the heap help is working at all.

The tool also can help to detect usage of invalid memory. For that it can fill
the newly allocated memory to increase the chances to get a crash and fine the
buggy place.

* `HHCONTENT=o ./my_app` - o = "original", memory is returned raw as is.

* `HHCONTENT=t ./my_app` - t = "trash", new memory will be filled with some
  trash bytes.
