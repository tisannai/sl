#ifndef SL_H
#define SL_H

/**
 * @file   sl.h
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Sat Jul  8 17:05:06 2017
 *
 * @brief  Simple String Library.
 *
 * SL is a small string library that has automatic string resizing and
 * fairly high performance. SL is compatible with standard C library
 * string functions.
 *
 * SL String looks like a "normal" C string, but it includes a hidden
 * descriptor. Descriptor is before the string content in
 * memory. Descriptor includes reservation size for the string storage,
 * and the effective length of the string. Effective length means string
 * length without the terminating null. SL Strings are always null
 * terminated.
 *
 * SL struct:
 *
 *        reservation (uint32_t)
 *        length      (uint32_t)
 *     -> content     (char*)
 *
 * SL datatype is "sls". It is a typedef of "char*", which means it is
 * usable by the standard library functions.
 *
 * SL library functions expect that "sls" typed arguments have the
 * "hidden" descriptor in place.
 *
 * Part of SL library functions mutate the string and some only reference
 * the string (read only access). Some mutating functions may need to
 * allocate more space to store the string, and these functions require
 * "slp" type to be used as argument type. "slp" is a pointer to
 * "sls". "slp" is needed since SL String might appear in different
 * memory address after the re-allocation. Even when the argument have to
 * be of "slp" type, the return value is still of "sls" type.
 *
 * Some SL library functions may use both SL or CSTR type
 * arguments. These functions use the "char*" argument type.
 *
 * Functions that convert CSTR to SL, use "_c" as postfix to designate
 * conversion. Note that even in this case you could replace CSTR with
 * SL, since SL is a superset of CSTR.
 *
 * Typically SL Strings are created with larger storage that what the
 * first content would require. For example we could create SL as:
 *
 *     sl = slsiz_c( "hello", 128 );
 *
 * Length would be 5 and allocation size 128. If we want to concatenate "
 * world!" to SL, we don't have to redo any allocations, since we have
 * spare storage. Concatenation would be done as:
 *
 *    slcat_c( &sl, " world!" );
 *
 * We could also create SL with minimum size:
 *
 *    sl = slstr_c( "hello" );
 *
 * Again the length would be 5, but storage would be only 6. If we now
 * concatenate the rest to SL, there will be a reallocation. After
 * re-allocation, the storage is still the minimum, i.e 5+7+1 = 13.
 *
 * Any heap allocated SL should be de-allocated after use.
 *
 *     sldel( &sl );
 *
 * De-allocation takes "slp" type argument in order to ensure that SL
 * becomes NULL after memory free.
 *
 * SL can also be used within stack allocated strings. First you have to
 * have some stack storage available.
 *
 *     char buf[ 128 ];
 *     sls sl;
 *
 * The you can take that into use with:
 *
 *     sl = sluse( buf, 128 );
 *
 * Note that stack allocated SL can't be enlarged, hence user must take
 * care that this does not happen. Also note that SL does not (in this
 * case) fit a string that has 127 characters (plus null). It will only
 * fit 119 plus null, since the descriptor takes 8 bytes of space.
 *
 * By default SL library uses malloc and friends to do heap
 * allocations. If you define SL_MEM_API, you can use your own memory
 * allocation functions.
 *
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>


/** Size type. */
typedef uint32_t sl_size_t;

/** SL structure. */
typedef struct
{
    sl_size_t res;      /**< Reservation. */
    sl_size_t len;      /**< Length (used). */
    char      str[ 0 ]; /**< String content. */
} sl_s;


/** Pointer to SL. */
typedef sl_s* sl;

/** Type for SL String. */
typedef char* sls;

/** Handle for mutable SL. */
typedef sls* slp;

/** SL array type. */
typedef sls* sla;


#ifdef SL_MEM_API

/*
 * SL_MEM_API allows to use custom memory allocation functions,
 * instead of the default: sl_malloc, sl_free, sl_realloc.
 *
 * If SL_MEM_API is used, the user must provide implementation for the
 * above functions and they must be compatible with malloc etc.
 *
 * Additionally user should compile the library by own means.
 */

extern void* sl_malloc( size_t size );
extern void sl_free( void* ptr );
extern void* sl_realloc( void* ptr, size_t size );

#else

/* Default to regular memory management functions. */

#define sl_malloc malloc
#define sl_free free
#define sl_realloc realloc

#endif



/* ------------------------------------------------------------
 * Library
 * ------------------------------------------------------------ */


/**
 * Create new storage for SL with given reservation size (i.e. at
 * least len+1).
 *
 * @param size Reservation size.
 *
 * @return SL.
 */
sls slnew( sl_size_t size );


/**
 * Use existing SL reservation with total size of "size".
 *
 * @param ss    Allocation for SL.
 * @param size  Allocation size.
 *
 * @return SL.
 */
sls sluse( void* ss, sl_size_t size );


/**
 * Delete SL.
 *
 * @param ss SLP.
 *
 * @return NULL
 */
sls sldel( slp ss );


/**
 * Create SL reservation for size. If current reservation is bigger,
 * do nothing.
 *
 * @param ss   SLP.
 * @param size Reservation size.
 *
 * @return SL.
 */
sls slres( slp ss, sl_size_t size );


/**
 * Shrink reservation to minimum size.
 *
 * @param ss SLP.
 *
 * @return SL.
 */
sls slmin( slp ss );


/**
 * Copy SL content from another SL.
 *
 * @param s1 SLP.
 * @param s2 SL.
 *
 * @return SL.
 */
sls slcpy( slp s1, sls s2 );


/**
 * Copy SL content from CSTR.
 *
 * @param s1 SLP.
 * @param s2 SL.
 *
 * @return SL.
 */
sls slcpy_c( slp s1, char* s2 );


/**
 * Fill (append) SL with "c" by "cnt" times.
 *
 * @param ss  SLP.
 * @param c   Char for filling.
 * @param cnt Fill count.
 *
 * @return SL.
 */
sls slfil( slp ss, char c, sl_size_t cnt );


/**
 * Duplicate SL, using same reservation as original.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls sldup( sls ss );


/**
 * Replicate (duplicate) SL, using mininum reservation.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls slrep( sls ss );


/**
 * Clear content of SL, i.e. set length to 0. Reservation is not
 * touched.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls slclr( sls ss );


/**
 * Create SL based on CSTR with size from CSTR.
 *
 * @param cs CSTR.
 *
 * @return SL.
 */
sls slstr_c( char* cs );


/**
 * Create SL based on CSTR with given size.
 *
 * Size is enlarged if CSTR is longer than given size.
 *
 * @param cs    CSTR.
 * @param size  Reservation size.
 *
 * @return SL.
 */
sls slsiz_c( char* cs, sl_size_t size );


/**
 * Return SL length.
 *
 * @param ss SL.
 *
 * @return Length.
 */
sl_size_t sllen( sls ss );


/**
 * Return SL allocation (reservation) size.
 *
 * @param ss SL.
 *
 * @return Reservation.
 */
sl_size_t slall( sls ss );


/**
 * Return SL base type.
 *
 * @param ss SL.
 *
 * @return Base type.
 */
sl slsl( sls ss );


/**
 * Return last character in SL.
 *
 * @param ss SL.
 *
 * @return Last char.
 */
char slend( sls ss );


/**
 * Compare two SL.
 *
 * @param s1 Reference SL.
 * @param s2 Compared SL.
 *
 * @return -1,0,1 (see strcmp).
 */
int slcmp( sls s1, sls s2 );


/**
 * Are two SLs different.
 *
 * @param s1 Reference SL.
 * @param s2 Compared SL.
 *
 * @return 1 if different.
 */
int sldff( sls s1, sls s2 );


/**
 * Sort SL array to alphabetical order.
 *
 * @param sa   SL array.
 * @param len  SL array length.
 */
void slsrt( sla sa, sl_size_t len );


/**
 * Concatenate SL to SL.
 *
 * @param s1 SLP.
 * @param s2 SL to add.
 *
 * @return SL.
 */
sls slcat( slp s1, sls s2 );


/**
 * Concatenate CSTR to SL.
 *
 * @param s1 SLP.
 * @param s2 CSTR to add.
 *
 * @return SL.
 */
sls slcat_c( slp s1, char* s2 );


/**
 * Push (insert) character to pos. Pos can be positive or negative.
 *
 * @param ss  SLP.
 * @param pos Pos.
 *
 * @return SL.
 */
sls slpsh( slp ss, int pos, char c );


/**
 * Pop (remove) character at pos. Pos can be positive or negative.
 *
 * @param ss  SL.
 * @param pos Pos.
 *
 * @return SL.
 */
sls slpop( sls ss, int pos );


/**
 * Cut to pos.
 *
 * @param ss  SL.
 * @param pos Pos.
 *
 * @return SL.
 */
sls sllim( sls ss, int pos );


/**
 * Cut off either end or start of string.
 *
 * With positive "cnt", cut off "cnt" characters from end.
 * With negative "cnt", cut off "cnt" characters from start.
 *
 * @param ss   SL.
 * @param cnt  Cut cnt.
 *
 * @return SL.
 */
sls slcut( sls ss, int cnt );


/**
 * Select a slice from SL and mutate SL. Positive index is from start
 * and negative from end of string. slsel() is not sensitive to the
 * order of boundaries. Ending index is exclusive.
 *
 * @param ss   SL.
 * @param a    A boundary.
 * @param b    B boundary.
 *
 * @return SL.
 */
sls slsel( sls ss, int a, int b );


/**
 * Insert SL into another SL.
 *
 * @param s1  Target SLP.
 * @param pos Insertion position.
 * @param s2  Inserted SL.
 *
 * @return Target.
 */
sls slins( slp s1, int pos, sls s2 );


/**
 * Insert CSTR into SL.
 *
 * @param s1  Target SLP.
 * @param pos Insertion position.
 * @param s2  Inserted CSTR.
 *
 * @return Target.
 */
sls slins_c( slp s1, int pos, char* s2 );


/**
 * Formatted (printf style) print to SL.
 *
 * @param ss   SLP.
 * @param fmt  Format.
 *
 * @return SL.
 */
sls slfmt( slp ss, char* fmt, ... );


/**
 * Variable Arguments (VA) version of slfmt().
 *
 * @param ss  SLP.
 * @param fmt Format.
 * @param ap  VA list.
 *
 * @return SL.
 */
sls slvpr( slp ss, char* fmt, va_list ap );


/**
 * Invert position, i.e. from positive index to negative and vice
 * versa. Logical position is not changed.
 *
 * @param ss  SL.
 * @param pos Pos.
 *
 * @return Inverted pos.
 */
int slinv( sls ss, int pos );


/**
 * Find char towards right.
 *
 * @param ss  SL.
 * @param c   Char to find.
 * @param pos Search start pos.
 *
 * @return Pos (or -1 if not found).
 */
int slfcr( sls ss, char c, sl_size_t pos );


/**
 * Find char towards left.
 *
 * @param ss  SL.
 * @param c   Char to find.
 * @param pos Search start pos.
 *
 * @return Pos (or -1 if not found).
 */
int slfcl( sls ss, char c, sl_size_t pos );


/**
 * Find "s2" from "s1". Return position or -1 if not found.
 *
 * "s2" can be SL or CSTR.
 *
 * @param s1  Base.
 * @param s2  Find.
 *
 * @return Pos (or -1 if not found).
 */
int slidx( sls s1, char* s2 );


/**
 * Divide (split) SL to pieces by character "c". Return the number of
 * pieces after split. Pieces are stored to array pointed by "div". If
 * character "c" hits the last char of SL, the last piece will be of
 * length 0.
 *
 * SL will be modified by replacing "c" with 0. This can be cancelled
 * with slswp() or user can use a duplicate in the first place.
 *
 * If called with "size" < 0, return only the number of parts. No
 * modification is done to SL.
 *
 * If called with "*div" != NULL, fill the preallocated div.
 *
 * Else calculate parts, and then allocate storage for it. Should be
 * freed by the user when done. "size" is then dont-care and will be
 * calculated for the user.
 *
 * @param ss   SL.
 * @param c    Char to split with.
 * @param size Size of div storage (-1 for na).
 * @param div  Address of div storage.
 *
 * @return Number of pieces.
 */
int sldiv( sls ss, char c, int size, char*** div );


/**
 * Same as sldiv() except segmentation (split) is done using CSTR.
 *
 * Both sldiv() and slseg() terminate the segment with single 0.
 *
 * @param ss   SL.
 * @param sc   CSTR to split with.
 * @param size Size of div storage (-1 for na).
 * @param div  Address of div storage.
 *
 * @return Number of pieces.
 */
int slseg( sls ss, char* sc, int size, char*** div );


/**
 * Glue (join) string array with string.
 *
 * @param sa   Str array.
 * @param size Str array size.
 * @param glu  Glue string.
 *
 * @return SL.
 */
sls slglu( sla sa, sl_size_t size, char* glu );


/**
 * Split "ss" into tokens delimited by "delim". sltok() is called
 * multiple times for each iteration separately. "pos" holds the state
 * between iterations.
 *
 * For first call "ss" should be itself and "*pos" should be NULL. For
 * subsequent calls only "*pos" will be considered.
 *
 * After last token, "*pos" will be set to "ss".
 *
 * Example:
 *   char* t, *pos, *delim = "XY";
 *   s = slstr_c( "abXYabcXYc" );
 *   pos = NULL;
 *   t = sltok( s, delim, &pos );
 *   t = sltok( s, delim, &pos );
 *   t = sltok( s, delim, &pos );
 *
 * @param ss     SL.
 * @param delim  Token delimiter.
 * @param pos    Iteration state.
 *
 * @return Start of current token (or NULL if no token).
 */
char* sltok( sls ss, char* delim, char** pos );


/**
 * Drop the extension "ext" from "ss".
 *
 * @param ss  SL.
 * @param ext Extension (i.e. file suffix).
 *
 * @return Updated SL (or NULL if no ext found).
 */
sls slext( sls ss, char* ext );


/**
 * Change to dirname, i.e. take out the basename.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls sldir( sls ss );


/**
 * Change to basename (file basename), i.e. take out the directory
 * part.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls slbas( sls ss );


/**
 * Swap (repair) SL by mapping "f" char to "t" char. Useful to cleanup
 * after sldiv() or slseg().
 *
 * @param ss SL.
 * @param f  From char.
 * @param t  To char.
 *
 * @return SL.
 */
sls slswp( sls ss, char f, char t );


/**
 * Map (replace) string "f" to "t" in "ss". "f" and "t" can be
 * either SL or CSTR.
 *
 * @param ss SLP.
 * @param f  From string.
 * @param t  To string.
 *
 * @return SL
 */
sls slmap( slp ss, char* f, char* t );


/**
 * Capitalize SL, i.e. upper case first letter.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls slcap( sls ss );


/**
 * Convert SL to upper case letters.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls sltou( sls ss );


/**
 * Convert SL to lower case letters.
 *
 * @param ss SL.
 *
 * @return SL.
 */
sls sltol( sls ss );


/**
 * Read complete file and return SL containing the file content.
 *
 * @param filename Name of file.
 *
 * @return SL.
 */
sls slrdf( char* filename );


/**
 * Write SL content to file.
 *
 * @param ss       SL with content.
 * @param filename File to write.
 *
 * @return SL.
 */
sls slwrf( sls ss, char* filename );


/**
 * Display SL content.
 *
 * @param ss SL.
 */
void slprn( sls ss );


#endif
