# SL - Simple String Library

SL is a small string library that has automatic string resizing and
fairly high performance. SL is compatible with standard C library
string functions.

SL String looks like a "normal" C string, but it includes a hidden
descriptor. Descriptor is before the string content in
memory. Descriptor includes reservation size for the string storage,
and the effective length of the string. Effective length means string
length without the terminating null. SL Strings are always null
terminated.

SL struct:

       reservation (uint32_t)
       length      (uint32_t)
    -> content     (char*)

SL datatype is "sls". It is a typedef of "char*", which means it is
usable by the standard library functions.

SL library functions expect that "sls" typed arguments have the
"hidden" descriptor in place.

Part of SL library functions mutate the string and some only reference
the string (read only access). Some mutating functions may need to
allocate more space to store the string, and these functions require
"slp" type to be used as argument type. "slp" is a pointer to
"sls". "slp" is needed since SL String might appear in different
memory address after the re-allocation. Even when the argument have to
be of "slp" type, the return value is still of "sls" type.

Some SL library functions may use both SL or CSTR type
arguments. These functions use the "char*" argument type.

Functions that convert CSTR to SL, use "_c" as postfix to designate
conversion. Note that even in this case you could replace CSTR with
SL, since SL is a superset of CSTR.

Typically SL Strings are created with larger storage that what the
first content would require. For example we could create SL as:

    sl = slsiz_c( "hello", 128 );

Length would be 5 and allocation size 128. If we want to concatenate "
world!" to SL, we don't have to redo any allocations, since we have
spare storage. Concatenation would be done as:

   slcat_c( &sl, " world!" );

We could also create SL with minimum size:

   sl = slstr_c( "hello" );

Again the length would be 5, but storage would be only 6. If we now
concatenate the rest to SL, there will be a reallocation. After
re-allocation, the storage is still the minimum, i.e 5+7+1 = 13.

Any heap allocated SL should be de-allocated after use.

    sldel( &sl );

De-allocation takes "slp" type argument in order to ensure that SL
becomes NULL after memory free.

SL can also be used within stack allocated strings. First you have to
have some stack storage available.

    char buf[ 128 ];
    sls sl;

The you can take that into use with:

    sl = sluse( buf, 128 );

Note that stack allocated SL can't be enlarged, hence user must take
care that this does not happen. Also note that SL does not (in this
case) fit a string that has 127 characters (plus null). It will only
fit 119 plus null, since the descriptor takes 8 bytes of space.

By default SL library uses malloc and friends to do heap
allocations. If you define SL_MEM_API, you can use your own memory
allocation functions.


Simple usage example:

    sls sl;

    /* Create SL with reservation size of 128 from CSTR ("hello"). */
    sl = slsiz_c( "hello", 128 );

    /* Concatenate SL with CSTR (" world!"). */
    slcat_c( &sl, " world!" );

    /* Concatenate SL with 10 'a' letters. */
    slfil( &sl, 'a', 10 );

    /* Get SL length. */
    length = sllen( sl );

    /* De-allocate SL after use. */
    sldel( &sl );


See "sl.h" for complete list of SL library API functions.


# Testing

Tests are in "test" directory. Tests are a reasonable example for SL
use. The boilerplate Ceedling files are not in GIT.

SL is tested with Ceedling. Execute

    shell> rake test:all

to run all tests. Execute

    shell> rake gcov:all

to generate coverage info, and finally, execute

    shell> rake utils:gcov

to generate coverage reports.
