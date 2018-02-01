# SL - Simple String Library

SL is a small string library that has automatic string storage
resizing and fairly high performance. SL is compatible with
standard C library string functions.

SL String looks like a "normal" C string, but it includes a hidden
descriptor. Descriptor is located just before string Content in
memory. Descriptor includes storage size of string Content, and
effective length of the string. Effective length means string
length without the terminating null. SL Strings are always null
terminated. The total SL Allocation is sum Descriptor allocation
plus the string storage allocation.

SL struct:

                      Field     Type          Addr
                      -----------------------------
     Descriptor ----- storage   (uint32_t)  | N + 0
                   `- length    (uint32_t)  | N + 4
        Content ----- string    (char*)     | N + 8

Basic SL datatype is "sls". Most SL library functions take it as
argument and they also return values in that type. "sls" is a
typedef of "char*", hence it is usable by standard C library
functions.

SL library functions expect that "sls" typed arguments have the
"hidden" descriptor in place. Using CSTR in place of a SL string
will cause memory corruption. CSTR has potentially "garbage" in
front of its first character in memory.

Part of the SL library functions mutate the string and some only
reference it (read only access). Some of the mutating functions may
need to allocate more space in order to fit the growing
string. These functions require "slp" type to be used as argument
type. "slp" is a pointer to "sls". "slp" is needed since SL String
might appear in different memory address after resizing. Even when
the argument have to be of "slp" type, the return value is still of
"sls" type. This makes use of SL more convenient.

Some SL library functions may use both SL and CSTR type
arguments. These functions use "char*" as argument type, since SL
is fully compatible with CSTR.

Functions that convert CSTR to SL, use "_c" as postfix to designate
conversion. Note that even in this case you could replace CSTR with
SL, since SL is a superset of CSTR.

Typically SL Strings are created with larger storage than what the
initial content would require. For example we could create SL as:

    ss = slsiz_c( "hello", 128 );

Length would be 5 and storage size 128. If we want to append "
world!" to SL, we don't have to redo any allocations, since we have
spare storage. Concatenation would be done as:

    slcat_c( &ss, " world!" );

We could also create SL with minimum size:

    ss = slstr_c( "hello" );

Again the length would be 5, but storage would be only 6. If we now
concatenate the rest to SL, there will be a re-allocation. After
re-allocation, the storage is still the minimum, i.e 5+7+1 = 13.

Any heap allocated SL should be de-allocated after use.

    sldel( &ss );

De-allocation takes "slp" type argument in order to ensure that SL
becomes NULL after memory free.

SL can also be used within stack allocated strings. First you have to
have some stack storage available.

    char buf[ 128 ];
    sls  ss;

Then you can take that into use with:

    ss = sluse( buf, 128 );

Note that stack allocated SL can't be enlarged, hence user must take
care that this does not happen. Also note that SL does not (in this
case) fit a string that has 127 characters (plus null). It will only
fit 119 plus null, since the descriptor takes 8 bytes of space.

By default SL library uses malloc and friends to do heap
allocations. If you define SL_MEM_API, you can use your own memory
allocation functions.

Custom memory function prototypes:
    void* sl_malloc ( size_t size );
    void  sl_free   ( void*  ptr  );
    void* sl_realloc( void*  ptr, size_t size );


Basic usage example:

    sls sl;

    /* Create SL from CSTR with storage size of 128. */
    sl = slsiz_c( "hello", 128 );

    /* Concatenate SL with CSTR. */
    slcat_c( &sl, " world!" );

    /* Get SL length (= 12). */
    length = sllen( sl );

    /* Append SL with 10 'a' letters. */
    slfil( &sl, 'a', 10 );

    /* Cut off the 'a' letters. */
    slcut( &sl, 12 );

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
