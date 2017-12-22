#ifndef SL_H
#define SL_H

/**
 * @file   sl.h
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Sat Jul  8 17:05:06 2017
 *
 * @brief  Simple to use string library.
 *
 * SL is a convenient string library that supports the most common
 * string operations. SL strings are used the same way as normal C
 * strings with C library functions.
 *
 * SL is represented by "sls" type (which is typedef'd from "char*")
 * and the descriptor info is "hidden". "char*" with descriptor is
 * called SL, and "normal" "char*" string is called CSTR. Address of
 * (pointer to) SL is called SLP. When API functions allow either
 * "char*" or "sls" type arguement, the "char*" is used.
 *
 * SL is defined as a data structure that holds:
 *   * Reservation for string storage, i.e. at least length + terminating 0.
 *   * Actual length of the string.
 *   * String content (i.e. the "sls" portion).
 *
 * SL functions return pointer to the String content (sls), so user
 * can use it as regular C-string. Internally SL library moves to SL
 * structure beginning and updates the descriptor accordingly and
 * finally returns the string content.
 *
 * SL struct:
 *      reservation (uint32_t)
 *      length      (uint32_t)
 *   -> content     (char*)
 *
 * The immutable functions take "char*" type parameters and mutable
 * functions take "char**" type parameters. "slp" is typedef of
 * "char**, and we assume that the descriptor part exists.
 *
 * Example:
 *   sls ss;
 *
 *   // Create SL with size 128 from CSTR "foobar".
 *   ss = slsiz_c( "foobar", 128 );
 *
 *   // Concatenate SL with CSTR "diiduu".
 *   slcat_c( &ss, "diiduu" );
 *
 *   // Concatenate SL with 10 'a' letters.
 *   slfil( &ss, 'a', 10 );
 *
 *   // Get SL length.
 *   length = sllen( ss );
 *
 *   // De-allocate SL after use.
 *   sldel( &ss );
 *
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>


/** Size type. */
typedef uint32_t sl_size_t;

/** SL structure. */
typedef struct {
  sl_size_t res;                /**< Reservation. */
  sl_size_t len;                /**< Length (used). */
  char  str[0];                 /**< String content. */
} sl_s;


/** Pointer to SL. */
typedef sl_s*    sl;

/** Handle for mutable SL. */
typedef char**   slp;

/** SL array type. */
typedef char**   sla;

/** Type for SL String. */
typedef char*   sls;


/** Select here or in compilation command. */
/* #define SL_MEM_API 1 */


#ifdef SL_MEM_API

/* SL_MEM_API allows to use custom memory allocation functions,
   instead of the default: malloc, free, realloc. */

/** Allocation function ptr type: malloc, galloc. */
typedef void* (*sl_malloc_t)(size_t);

/** Allocation function ptr type: free. */
typedef void (*sl_free_t)(void*);

/** Allocation function ptr type: realloc. */
typedef void* (*sl_realloc_t)(void*,size_t);

#else

/* Default to regular memmgmt functions. */

# define sl_malloc   malloc
# define sl_free     free
# define sl_realloc  realloc

#endif



/* ------------------------------------------------------------
 * Library
 * ------------------------------------------------------------ */


/**
 * Configure allocation functions.
 *
 * @param sl_malloc  Allocator.
 * @param sl_free    Free.
 * @param sl_realloc Realloc.
 */
#ifdef SL_MEM_API
void sl_cfg_alloc( sl_malloc_t  malloc,
                   sl_free_t    free,
                   sl_realloc_t realloc );
#endif


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
sls slcpy  ( slp s1, sls s2 );


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
sls slcat  ( slp s1, sls s2 );


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
 * Cut either end or start of string. With positive "pos", cut from
 * end by pos. With negative "pos", cut from start upto "pos"
 * (exclusive).
 *
 * @param ss   SL.
 * @param pos  Cut pos.
 *
 * @return SL.
 */
sls slcut( sls ss, int pos );


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
sls slins  ( slp s1, int pos, sls s2 );


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
 * Glue (join) CSTR array with CSTR.
 *
 * @param sa   Str array.
 * @param size Str array size.
 * @param glu  Glue CSTR.
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


#ifdef SL_MEM_API

/**
 * Standard malloc for SL library.
 *
 * @param size Allocation size.
 *
 * @return Allocation.
 */
void* sl_malloc_f( size_t size );


/**
 * Standard free for SL library.
 *
 * @param ptr Allocation to free.
 */
void sl_free_f( void* ptr );


/**
 * Standard re-allocator for SL library.
 *
 * @param ptr   Existing allocation.
 * @param size  Size for new allocation.
 *
 * @return New allocation.
 */
void* sl_realloc_f( void* ptr, size_t size );

#endif // SL_MEM_API


#endif
