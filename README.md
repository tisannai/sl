# SL - Simple String Library

SL is a convenient string library that supports the most common string
operations. SL strings are used the same way as normal C strings with
C library functions.

SL is represented by "sls" type (which is typedef'd from "char*") and
the descriptor info is "hidden". "char*" with descriptor is called SL,
and "normal" "char*" string is called CSTR. Address of (pointer to) SL
is called SLP. When API functions allow either "char*" or "sls" type
arguement, the "char*" is used.

SL is defined as a data structure that holds:
  * Reservation for string storage, i.e. at least length + terminating 0.
  * Actual length of the string.
  * String content (i.e. the "sls" portion).

SL functions return pointer to the String content (sls), so user
can use it as regular C-string. Internally SL library moves to SL
structure beginning and updates the descriptor accordingly and
finally returns the string content.

SL struct:
       reservation (uint32_t)
       length      (uint32_t)
    -> content     (char*)

The immutable functions take "char*" type parameters and mutable
functions take "char**" type parameters. "slp" is typedef of "char**",
and we assume that the descriptor part exists.

See "sl.h" for SL library API functions.


Example:

    sls ss;

    // Create SL with size 128 from CSTR "foobar".
    ss = slsiz_c( "foobar", 128 );

    // Concatenate SL with CSTR "diiduu".
    slcat_c( &ss, "diiduu" );

    // Concatenate SL with 10 'a' letters.
    slfil( &ss, 'a', 10 );

    // Get SL length.
    length = sllen( ss );

    // De-allocate SL after use.
    sldel( &ss );



# Testing

Tests are in "test" directory. Tests are a reasonable example for SL
use.

SL is tested with Ceedling. Execute

    shell> rake test:all

to run all tests. Execute

    shell> rake gcov:all

to generate coverage info, and finally, execute

    shell> rake utils:gcov

to generate coverage reports.
