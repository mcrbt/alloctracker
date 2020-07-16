# alloctracker


## Description

`alloctracker` is a small object file that can be added to any **plain C**
project to detect dynamic memory issues, especially *memory leaks*.

`alloctracker` works the following way. *Standard library* functions
that are known to allocate dynamic memory on the heap are overridden in order
to track those allocations. In addition to their original functionality a
dynamic linked list is created keeping pointers and sizes of allocation
requests for dynamic memory by the code linked against `alloctracker`.
A second list holds file handles, their name and access mode. The lists
are dynamically decomposed as resources are released. All items not cleared
at program termination can be reported. Unfreed dynamic memory is considered
*leaked*.


## Use case

`alloctracker` is intended to help during the *debugging stage*. Listing
the source code locations where memory is allocated and files are opened
may give the crucial hint to where those memory should be freed again
(or files should be closed, respectively).


## Overridden library functions

As already mentioned, `alloctracker` "overrides" some C standard library
functions to implement their tracking. The following function calls are
tracked:

- `malloc`    (`stdlib.h`)
- `calloc`    (`stdlib.h`)
- `realloc`   (`stdlib.h`)
- `free`      (`stdlib.h`)
- `fopen`     (`stdio.h`)
- `freopen`   (`stdio.h`)
- `tmpfile`   (`stdio.h`)
- `fclose`    (`stdio.h`)
- `strdup`    (`string.h`)
- `getline`   (`stdio.h`)
- `getdelim`  (`stdio.h`)


The functions `strdup`, `getline`, and `getdelim`, are not available with
all C standards and/or platforms (see section *C standard compliance*).
However, all functions should be available on *GNU/Linux* operating systems
using the C standard *GNU99* (GCC option `-std=gnu99`).

<br/>

(**NOTE**: It is not possible on all platforms to *re*-open (`freopen`) a
temporary file (created with `tmpfile`). This may be due to restrictions
on changing the file access mode of temporary files, or simply because
temporary files created with `tmpfile` are automatically deleted when the
file is closed (using `fclose`). *This is not a bug of `alloctracker`*.)


## Compilation

For performance reasons it is recommended to compile `alloctracker` right
into the target software. That is, link the compiled object file statically
against the target.

The provided *Makefile* can be used to compile `alloctracker` to a sole
object file `obj/alloctracker.o`.

```
$ make
```

### Demonstration

Some basic demonstration code code is contained in `src/at_test.c` to show
`alloctracker` in action. `at_test.c` also acts as an example on how to apply
`alloctracker`. The demonstration code can be compiled using the following
command, where `alloctracker` is automatically linked.

```
$ make test
```

The respective executable is placed under the `./bin` directory and can be
run with no arguments by:

```
$ bin/alloctracker_test
```


## Application

If the *preprocessor macro* "`AT_ALLOC_TRACK`" is defined at compile
time a report may be printed to the STDERR (*standard error stream*,
file descriptor 2) on demand, listing all unfreed memory blocks including
their size and the source code location where the allocation is
issued. Additionally, all files opened by the program and not closed until
termination are printed with their name (if available), their access mode,
and the source code location where the file is opened.

A file's name is be unavailable if it is a *temporary file* created with
the standard library function `tmpfile`, declared in the header file
`stdio.h`.

To void the need of altering Makefiles (i.e. the whole build process) for
production releases in order to *not link* this `alloctracker`'s object file,
the preprocessor macro `AT_ALLOC_TRACK` can simply be undefined, or omitted,
respectively. In that case the `alloctracker` code is not compiled into the
final build. The actual source code of the software does not need to be
changed either. If `alloctracker` *function macros* (see below) are called,
then they will simply do nothing, at all.

Previously mentioned "*function macros*" that can be used in the source
code are:

- `AT_REPORT`:      print all unfreed memory blocks and unclosed files
                    to STDERR along with some system resource usage
                    statistics
- `AT_FREE_ALL`:    free all unfreed dynamic memory allocated on the heap,
                    and close any open files left

A small demonstration code (`src/at_test.c`) is provided (see section
*Demonstration*).

**NOTE**: To be able to print the report `AT_FREE_ALL` **must not** be called
before `AT_REPORT`. Otherwise nothing is printed when calling `AT_REPORT`.

<br/>

To use `alloctracker` with an arbitrary C project two files are required
(after compiling `alloctracker`):

* `src/alloctracker.h` and
* `obj/alloctracker.o`.

Two things are *crucial* for `alloctracker` to work correctly:

1. The header file `alloctracker.h` needs to be included
   **in all of the source files** of the target software
   (`#include "alloctracker.h"`).
2. It is very important to let this include statement be the
   **very last one** of all includes in each source file!

Additionally, when compiling the target software the *preprocessor macro*
"`AT_ALLOC_TRACK`" needs to be defined using the GCC command line option
"`-DAT_ALLOC_TRACK`". This is done in the Makefile of the project when
building the demonstration code (`$ make test`).
For not using `alloctracker` in production this flag can simply be omitted.


## Configuration

When displaying the report of memory leaks and open files, long filenames
are truncated. The default behavior is to truncate these names *from the
front* to enable distinction on their filename extension. The revert the
default behavior the preprocessor macro `AT_TRUNCATE_BACK` can be defined
at compile time and set to a value greater than 0. This causes `alloctracker`
to truncate long filenames from the back.
An ellipsis is put on the truncated end of the name to indicate truncation.


## Verification

In order to verify the detection of lost heap allocations the software
`valgrind` can be used. It implements a more technical approach to the
problem and thus yields far more reliable results as it does not depend on
overridden library functions and different C standards, and language, or
compiler extensions, respectively. Consequently, `valgrind` is able to detect
memory blocks of invalid size, for instance.

For more information on the `valgrind` tool suite take a look at its homepage
[Valgrind](https://www.valgrind.org). Officially `valgrind` is only
distributed as source code, such as
[here](https://www.valgrind.org/downloads/current.html#current).

<br/>

Verifying the behavior of `alloctracker` using `valgrind` is as simple as
executing the following command:

```bash
$ valgrind bin/alloctracker_test
```

`valgrind` and `alloctracker` yield different resource usage statistics
(e.g. the overall number of allocated bytes), because `alloctracker`
intentionally does not record its own allocations for maintaining the dynamic
lists (see section *Description*) while `valgrind` does, obviously.
Anyway `valgrind` should report "`in use at exit: 0 bytes in 0 blocks`" and
"`0 errors from 0 contexts (suppressed: 0 from 0)`".


## C standard compliance

`alloctracker` is syntactically compliant to the *ANSI C standard* (*C89*,
or *C90*, respectively). This means it can be compiled for the ANSI standard
with the GCC options `-ansi` without any warnings, allowing `alloctracker`
to be linked against any C project that is ANSI compliant itself, as well.
Respective *feature test macros* are used to exclude source code not being
available in the requested C standard.

Some of the functions `alloctracker` overrides, are *GNU extensions*.
Affected functions protected by feature test macros are `strdup`, `getline`,
and `getdelim`, as well as the *builtin* `__func__` (for substituting the
current function's name). On GNU/Linux systems the function `strdup` may be
available from the C standard *C99* (GCC option `-std=c99`) onward, while
`getline` and `getdelim` at least require the standard *GNU99* (`-std=gnu99`).

To ensure the demonstration code shows all functionality the Makefile defines
the C standard as `-std=gnu99`. For other projects applying `alloctracker` the
standard passed to the compiler can thus be adapted.

<br/>

(As `alloctracker` uses `strdup` itself, the function is defined for private
use in this project, if it is not available elsewhere. To remain compliant
to any applied C standard `strdup` is not made available to the target
software, in case it is not available for this applied standard.)


## Copyright

Copyright &copy; 2019-2020 Daniel Haase

`alloctracker` is licensed under the **BOOST software license**, version 1.

The project is inspired by code written by my colleague Jan Z. at
*Q-vi Tech GmbH*.


## License

```
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
```
