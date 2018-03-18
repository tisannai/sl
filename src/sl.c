/**
 * @file   sl.c
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Sat Jul  8 17:05:06 2017
 *
 * @brief  Simple String Library.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "sl.h"



/* ------------------------------------------------------------
 * Utility macros
 * ------------------------------------------------------------ */

/* clang-format off */
#define sl_malsize(s)  (sizeof(sl_s) + s)

#define sl_str(s)      ((char*)&(s->str[0]))
#define sl_base(s)     ((sl_base_p)((s)-(sizeof(sl_s))))
#define sl_len(s)      (((sl_base_p)((s)-(sizeof(sl_s))))->len)
#define sl_len1(s)     ((((sl_base_p)((s)-(sizeof(sl_s))))->len)+1)
#define sl_res(s)      (((sl_base_p)((s)-(sizeof(sl_s))))->res)
#define sl_end(s)      ((char*)((s)+sl_len(s)))

#define sc_len(s)      strlen(s)
#define sc_len1(s)     (strlen(s)+1)
/* clang-format on */



/* ------------------------------------------------------------
 * Utility functions.
 * ------------------------------------------------------------ */

static char* sl_copy_setup( char* dst, char* src );
static off_t sl_file_size( const char* filename );
static sl_size_t sl_norm_idx( sl_t ss, int idx );
static sl_t sl_copy_base( sl_p s1, char* s2, sl_size_t len1 );
static int sl_compare_base( const void* s1, const void* s2 );
static sl_t sl_concatenate_base( sl_p s1, char* s2, sl_size_t len1 );
static sl_t sl_insert_base( sl_p s1, int pos, char* s2, sl_size_t len1 );
static int sl_divide_base( sl_t ss, char c, int size, char** div );
static int sl_segment_base( sl_t ss, char* sc, int size, char** div );

static sl_size_t sl_u64_str_len( uint64_t u64 );
static void sl_u64_to_str( uint64_t u64, char* str );
static sl_size_t sl_i64_str_len( int64_t i64 );
static void sl_i64_to_str( int64_t i64, char* str );



/* ------------------------------------------------------------
 * Library
 * ------------------------------------------------------------ */

sl_t sl_new( sl_size_t size )
{
    sl_base_p s;
    s = (sl_base_p)sl_malloc( sl_malsize( size ) );
    s->res = size;
    s->len = 0;
    s->str[ 0 ] = 0;
    return sl_str( s );
}


sl_t sl_use( void* mem, sl_size_t size )
{
    sl_base_p s = mem;
    s->res = size - sizeof( sl_s );
    s->len = 0;
    s->str[ 0 ] = 0;
    return sl_str( s );
}


sl_t sl_del( sl_p sp )
{
    sl_free( sl_base( *sp ) );
    *sp = 0;
    return NULL;
}


sl_t sl_reserve( sl_p sp, sl_size_t size )
{
    if ( sl_res( *sp ) < size ) {
        sl_base_p s;
        s = sl_base( *sp );
        s = (sl_base_p)sl_realloc( s, sl_malsize( size ) );
        s->res = size;
        *sp = sl_str( s );
    }

    return *sp;
}


sl_t sl_compact( sl_p sp )
{
    sl_size_t len = sl_len1( *sp );

    if ( sl_res( *sp ) > len ) {
        sl_base_p s;
        s = sl_base( *sp );
        s = (sl_base_p)sl_realloc( s, sl_malsize( len ) );
        s->res = len;
        *sp = sl_str( s );
    }

    return *sp;
}


sl_t sl_copy( sl_p s1, sl_t s2 )
{
    return sl_copy_base( s1, s2, sl_len1( s2 ) );
}


sl_t sl_copy_c( sl_p s1, char* s2 )
{
    return sl_copy_base( s1, s2, sc_len1( s2 ) );
}


sl_t sl_fill_with_char( sl_p sp, char c, sl_size_t cnt )
{
    ssize_t len = sl_len( *sp );
    sl_reserve( sp, len + cnt + 1 );
    char* p = &( ( *sp )[ len ] );
    for ( sl_size_t i = 0; i < cnt; i++, p++ )
        *p = c;
    *p = 0;
    sl_len( *sp ) += cnt;
    return *sp;
}


sl_t sl_multiple_str_append( sl_p sp, char* cs, sl_size_t cnt )
{
    ssize_t len = sl_len( *sp );
    ssize_t clen = sc_len( cs );

    sl_reserve( sp, len + cnt * clen + 1 );
    char* p = &( ( *sp )[ len ] );
    for ( sl_size_t i = 0; i < cnt; i++ ) {
        strncpy( p, cs, clen );
        p += clen;
    }
    *p = 0;
    sl_len( *sp ) += cnt * clen;
    return *sp;
}


sl_t sl_duplicate( sl_t ss )
{
    sl_t sn;
    sn = sl_new( sl_res( ss ) );
    sl_copy( &sn, ss );
    return sn;
}


char* sl_duplicate_c( sl_t ss )
{
    ssize_t len1 = sl_len1( ss );
    char* dup = sl_malloc( len1 );
    strncpy( dup, ss, len1 );
    return dup;
}


sl_t sl_replicate( sl_t ss )
{
    sl_t sn;
    sn = sl_new( sl_len( ss ) + 1 );
    sl_copy( &sn, ss );
    return sn;
}


sl_t sl_clear( sl_t ss )
{
    sl_len( ss ) = 0;
    *ss = 0;
    return ss;
}


sl_t sl_from_str_c( char* cs )
{
    sl_size_t len = sc_len1( cs );
    sl_t       ss = sl_new( len );
    strncpy( ss, cs, len );
    sl_len( ss ) = len - 1;
    return ss;
}


sl_t sl_from_str_with_size_c( char* cs, sl_size_t size )
{
    sl_size_t len = sc_len1( cs );
    sl_t       ss;
    if ( size > len )
        ss = sl_new( size );
    else
        ss = sl_new( len );
    sl_copy_c( &ss, cs );
    return ss;
}


sl_size_t sl_length( sl_t ss )
{
    return sl_len( ss );
}


sl_size_t sl_reservation_size( sl_t ss )
{
    return sl_res( ss );
}


sl_base_p sl_base_ptr( sl_t ss )
{
    return sl_base( ss );
}


char sl_end_char( sl_t ss )
{
    if ( sl_len( ss ) == 0 )
        return 0;
    else
        return ss[ sl_len( ss ) - 1 ];
}


int sl_compare( sl_t s1, sl_t s2 )
{
    return strcmp( s1, s2 );
}


int sl_is_different( sl_t s1, sl_t s2 )
{
    if ( sl_len( s1 ) != sl_len( s2 ) )
        return 1;
    else if ( strcmp( s1, s2 ) == 0 )
        return 0;
    else
        return 1;
}


void sl_sort( sl_v sa, sl_size_t len )
{
    qsort( sa, len, sizeof( char* ), sl_compare_base );
}


sl_t sl_concatenate( sl_p s1, sl_t s2 )
{
    return sl_concatenate_base( s1, s2, sl_len1( s2 ) );
}


sl_t sl_concatenate_c( sl_p s1, char* s2 )
{
    return sl_concatenate_base( s1, s2, sc_len1( s2 ) );
}


sl_t sl_push_char_to( sl_p sp, int pos, char c )
{
    pos = sl_norm_idx( *sp, pos );
    sl_base_p s = sl_base( *sp );
    sl_reserve( sp, sl_len( *sp ) + 1 );
    if ( (sl_size_t)pos != s->len )
        memmove( &s->str[ pos + 1 ], &s->str[ pos ], s->len - pos );
    s->str[ pos ] = c;
    s->len++;
    s->str[ s->len ] = 0;
    return *sp;
}


sl_t sl_pop_char_from( sl_t ss, int pos )
{
    pos = sl_norm_idx( ss, pos );
    sl_base_p s = sl_base( ss );
    if ( (sl_size_t)pos != s->len ) {
        memmove( &s->str[ pos ], &s->str[ pos + 1 ], s->len - pos );
        s->len--;
    }
    return ss;
}


sl_t sl_limit_to_pos( sl_t ss, int pos )
{
    sl_base_p s = sl_base( ss );
    s->str[ pos ] = 0;
    s->len = pos;
    return ss;
}


sl_t sl_cut( sl_t ss, int cnt )
{
    int pos;
    sl_base_p s = sl_base( ss );
    if ( cnt >= 0 ) {
        pos = s->len - cnt;
        s->str[ pos ] = 0;
        s->len = pos;
        return ss;
    } else {
        sl_size_t len = s->len + cnt;
        pos = -cnt;
        memmove( s->str, &s->str[ pos ], len );
        s->len = len;
        s->str[ len ] = 0;
        return ss;
    }
}


sl_t sl_select_slice( sl_t ss, int a, int b )
{
    sl_size_t an, bn;

    /* Normalize a. */
    an = sl_norm_idx( ss, a );

    /* Normalize b. */
    bn = sl_norm_idx( ss, b );

    /* Reorder. */
    if ( bn < an ) {
        int t;
        t = an;
        an = bn;
        bn = t;
    }

    sl_base_p s = sl_base( ss );
    memmove( s->str, &s->str[ an ], bn - an );
    s->str[ bn - an ] = 0;
    s->len = bn - an;

    return ss;
}


sl_t sl_insert_to( sl_p s1, int pos, sl_t s2 )
{
    return sl_insert_base( s1, pos, s2, sl_len1( s2 ) );
}


sl_t sl_insert_to_c( sl_p s1, int pos, char* s2 )
{
    return sl_insert_base( s1, pos, s2, sc_len1( s2 ) );
}


sl_t sl_format( sl_p sp, char* fmt, ... )
{
    sl_t     ret;
    va_list ap;

    va_start( ap, fmt );
    ret = sl_va_format( sp, fmt, ap );
    va_end( ap );

    return ret;
}


sl_t sl_va_format( sl_p sp, char* fmt, va_list ap )
{
    va_list coap;

    /* Copy ap to coap for second va-call. */
    va_copy( coap, ap );

    int size;
    size = vsnprintf( NULL, 0, fmt, ap );

    if ( size < 0 )
        return NULL; // GCOV_EXCL_LINE

    size++;
    sl_reserve( sp, sl_len( *sp ) + size );

    size = vsnprintf( sl_end( *sp ), size, fmt, coap );
    va_end( coap );

    sl_len( *sp ) += size;

    return *sp;
}


sl_t sl_format_quick( sl_p sp, char* fmt, ... )
{
    sl_t     ret;
    va_list ap;

    va_start( ap, fmt );
    ret = sl_va_format_quick( sp, fmt, ap );
    va_end( ap );

    return ret;
}


sl_t sl_va_format_quick( sl_p sp, char* fmt, va_list ap )
{
    va_list coap;

    /* Copy ap to coap for second va-call. */
    va_copy( coap, ap );

    int size = 0;


    /* ------------------------------------------------------------
     * Calculate string size.
     */

    char*    c;
    char*    ts;
    int64_t  i64;
    uint64_t u64;

    c = fmt;

    while ( *c ) {

        switch ( *c ) {

            case '%': {
                c++;

                switch ( *c ) {

                    case 's': {
                        ts = va_arg( ap, char* );
                        size += strlen( ts );
                        break;
                    }

                    case 'S': {
                        ts = va_arg( ap, char* );
                        size += sl_len( ts );
                        break;
                    }

                    case 'i': {
                        i64 = va_arg( ap, int );
                        size += sl_i64_str_len( i64 );
                        break;
                    }

                    case 'I': {
                        i64 = va_arg( ap, int64_t );
                        size += sl_i64_str_len( i64 );
                        break;
                    }

                    case 'u': {
                        u64 = va_arg( ap, unsigned int );
                        size += sl_u64_str_len( u64 );
                        break;
                    }

                    case 'U': {
                        u64 = va_arg( ap, uint64_t );
                        size += sl_u64_str_len( u64 );
                        break;
                    }

                    case 'c': {
                        size++;
                        break;
                    }

                    case '%': {
                        size++;
                        break;
                    }

                    default: {
                        size++;
                        break;
                    }
                }

                c++;
                break;
            }

            default: {
                size++;
                c++;
                break;
            }
        }
    }

    va_end( ap );

    sl_reserve( sp, sl_len1( *sp ) + size );


    /* ------------------------------------------------------------
     * Expand format string.
     */

    char* wp = sl_end( *sp );

    sl_len( *sp ) += size;
    c = fmt;

    while ( *c ) {

        switch ( *c ) {

            case '%': {
                c++;

                switch ( *c ) {

                    case 's':
                    case 'S': {
                        ts = va_arg( coap, char* );
                        if ( *c == 's' )
                            size = strlen( ts );
                        else
                            size = sl_len( ts );
                        strncpy( wp, ts, size );
                        wp += size;
                        break;
                    }

                    case 'i':
                    case 'I': {
                        if ( *c == 'i' )
                            i64 = va_arg( coap, int );
                        else
                            i64 = va_arg( coap, int64_t );
                        size = sl_i64_str_len( i64 );
                        sl_i64_to_str( i64, wp );
                        wp += size;
                        break;
                    }

                    case 'u':
                    case 'U': {
                        if ( *c == 'u' )
                            u64 = va_arg( coap, unsigned int );
                        else
                            u64 = va_arg( coap, uint64_t );
                        size = sl_u64_str_len( u64 );
                        sl_u64_to_str( u64, wp );
                        wp += size;
                        break;
                    }

                    case 'c': {
                        char ch;
                        ch = (char)va_arg( coap, int );
                        *wp++ = ch;
                        break;
                    }

                    case '%': {
                        *wp++ = '%';
                        break;
                    }

                    default: {
                        *wp++ = *c;
                        break;
                    }
                }

                c++;
                break;
            }

            default: {
                *wp++ = *c++;
                break;
            }
        }
    }
    va_end( coap );

    *wp = 0;

    return *sp;
}


int sl_invert_pos( sl_t ss, int pos )
{
    if ( pos > 0 )
        return -1 * ( sl_len( ss ) - pos );
    else
        return sl_len( ss ) + pos;
}


int sl_find_char_right( sl_t ss, char c, sl_size_t pos )
{
    while ( pos < sl_len( ss ) && ss[ pos ] != c )
        pos++;

    if ( pos == sl_len( ss ) )
        return -1;
    else
        return pos;
}


int sl_find_char_left( sl_t ss, char c, sl_size_t pos )
{
    while ( pos > 0 && ss[ pos ] != c )
        pos--;

    if ( pos == 0 && ss[ pos ] != c )
        return -1;
    else
        return pos;
}


int sl_find_index( sl_t s1, char* s2 )
{
    if ( s2[ 0 ] == 0 )
        return -1;

    int i1 = 0;
    int i2 = 0;
    int i;

    while ( s1[ i1 ] ) {
        i = i1;
        if ( s1[ i ] == s2[ i2 ] ) {
            while ( s1[ i ] == s2[ i2 ] && s2[ i2 ] ) {
                i++;
                i2++;
            }
            if ( s2[ i2 ] == 0 )
                return i1;
        }
        i1++;
    }

    return -1;
}


int sl_divide_with_char( sl_t ss, char c, int size, char*** div )
{
    if ( size < 0 ) {
        /* Just count size, don't replace chars. */
        return sl_divide_base( ss, c, -1, NULL );
    } else if ( *div ) {
        /* Use pre-allocated storage. */
        return sl_divide_base( ss, c, size, *div );
    } else {
        /* Calculate size and allocate storage. */
        size = sl_divide_base( ss, c, -1, NULL );
        *div = (char**)sl_malloc( size * sizeof( char* ) );
        return sl_divide_base( ss, c, size, *div );
    }
}


int sl_segment_with_str( sl_t ss, char* sc, int size, char*** div )
{
    if ( size < 0 ) {
        /* Just count size, don't replace chars. */
        return sl_segment_base( ss, sc, -1, NULL );
    } else if ( *div ) {
        /* Use pre-allocated storage. */
        return sl_segment_base( ss, sc, size, *div );
    } else {
        /* Calculate size and allocate storage. */
        size = sl_segment_base( ss, sc, -1, NULL );
        *div = (char**)sl_malloc( size * sizeof( char* ) );
        return sl_segment_base( ss, sc, size, *div );
    }
}


sl_t sl_glue_array( sl_v sa, sl_size_t size, char* glu )
{
    int       len = 0;
    sl_size_t i;

    /* Calc sa len. */
    i = 0;
    while ( i < size )
        len += strlen( sa[ i++ ] );

    /* Add glu len. */
    len += ( size - 1 ) * strlen( glu );

    sl_t ss;
    ss = sl_new( len + 1 );
    sl_len( ss ) = len;

    /* Build result. */
    char* p = ss;
    i = 0;
    while ( i < size ) {
        p = sl_copy_setup( p, sa[ i ] );
        if ( i < ( size - 1 ) )
            p = sl_copy_setup( p, glu );
        i++;
    }
    ss[ sl_len( ss ) ] = 0;

    return ss;
}


char* sl_tokenize( sl_t ss, char* delim, char** pos )
{
    if ( *pos == 0 ) {
        /* First iteration. */
        int idx;
        idx = sl_find_index( ss, delim );
        if ( idx < 0 )
            return NULL;
        else {
            ss[ idx ] = 0;
            *pos = &( ss[ idx ] );
            return ss;
        }
    } else if ( *pos == sl_end( ss ) ) {
        /* Passed the last iteration. */
        return NULL;
    } else {
        /* Fix nulled char. */
        **pos = delim[ 0 ];
        char* p = *pos;
        p += strlen( delim );

        if ( *p == 0 ) {
            /* ss ended with delim, so we are done. */
            *pos = sl_end( ss );
            return NULL;
        }

        /* Find next delim. */
        int idx;
        idx = sl_find_index( p, delim );
        if ( idx < 0 ) {
            /* Last token, mark this by: */
            *pos = sl_end( ss );
            return p;
        } else {
            p[ idx ] = 0;
            *pos = &( p[ idx ] );
            return p;
        }
    }
}


sl_t sl_rm_extension( sl_t ss, char* ext )
{
    char* pos;
    char* t;

    pos = NULL;
    t = sl_tokenize( ss, ext, &pos );
    if ( t ) {
        sl_len( ss ) = pos - ss;
        return ss;
    } else
        return NULL;
}


sl_t sl_directory_name( sl_t ss )
{
    int i;

    /* Find first "/" from end. */
    i = sl_len( ss );
    while ( i > 0 && ss[ i ] != '/' )
        i--;

    if ( i == 0 ) {
        if ( ss[ i ] == '/' ) {
            ss[ 1 ] = 0;
            sl_len( ss ) = 1;
        } else {
            ss[ 0 ] = '.';
            ss[ 1 ] = 0;
            sl_len( ss ) = 1;
            return ss;
        }
    } else {
        ss[ i ] = 0;
        sl_len( ss ) = i;
    }

    return ss;
}


sl_t sl_basename( sl_t ss )
{
    int i;

    /* Find first "/" from end. */
    i = sl_len( ss );
    while ( i > 0 && ss[ i ] != '/' )
        i--;

    if ( i == 0 && ss[ i ] != '/' ) {
        return ss;
    } else {
        i++;
        sl_len( ss ) = sl_len( ss ) - i;
        memmove( ss, &( ss[ i ] ), sl_len( ss ) );
        ss[ sl_len( ss ) ] = 0;
    }

    return ss;
}


sl_t sl_swap_chars( sl_t ss, char f, char t )
{
    sl_size_t i;

    i = 0;
    while ( i < sl_len( ss ) ) {
        if ( ss[ i ] == f )
            ss[ i ] = t;
        i++;
    }

    return ss;
}


sl_t sl_map_str( sl_p sp, char* f, char* t )
{
    /*
     * If "t" is longer than "f", loop and count how many instances of
     * "f" is found. Increase size of sp by N*t.len - N*f.len.
     *
     * Loop and find the next "f" index. Skip upto index and insert "t"
     * inplace. Continue until "f" is no more found and insert tail of
     * "sp".
     */

    sl_size_t f_len = sc_len( f );
    sl_size_t t_len = sc_len( t );

    int   idx;
    char *a, *b;

    if ( t_len > f_len ) {
        /* Calculate number of parts. */
        sl_size_t cnt = 0;

        /*
         * Replace XXX with YYYY.
         *
         * foooXXXfiiiXXXdiii
         * foooYYYYfiiiYYYYdiii
         *
         * Prepare org before copy as:
         * --foooXXXfiiiXXXdiii
         *
         *   OR
         *
         * foooXXXfiiiXXXdiiiXXX
         * foooYYYYfiiiYYYYdiiiYYYY
         */
        a = *sp;

        while ( 1 ) {
            idx = sl_find_index( a, f );
            if ( idx >= 0 ) {
                cnt++;
                a += ( idx + f_len );
            } else {
                break;
            }
        }

        sl_size_t nlen;
        sl_size_t olen = sl_len( *sp );
        nlen = sl_len( *sp ) - ( cnt * f_len ) + ( cnt * t_len );
        sl_reserve( sp, nlen + 1 );
        sl_len( *sp ) = nlen;

        /*
         * Shift original sp content to right in order to enable copying
         * chars safely from right to left.
         */
        b = &( ( *sp )[ nlen - olen ] );
        memmove( b, *sp, olen + 1 );
        a = *sp;
    } else {
        /*
         * Replace XXX with YY.
         *
         * foooXXXfiiiXXXdiii
         * foooYYfiiiYYdiii
         */
        a = *sp;
        b = *sp;
    }

    while ( *b ) {
        idx = sl_find_index( b, f );
        if ( idx >= 0 ) {
            strncpy( a, b, idx );
            a += idx;
            a = sl_copy_setup( a, t );
            b += ( idx + f_len );
        } else {
            a = sl_copy_setup( a, b );
            *a = 0;
            break;
        }
    }

    if ( *b == 0 )
        *a = 0;

    return *sp;
}


sl_t sl_capitalize( sl_t ss )
{
    if ( sl_len( ss ) > 0 )
        ss[ 0 ] = toupper( ss[ 0 ] );

    return ss;
}


sl_t sl_toupper( sl_t ss )
{
    for ( sl_size_t i = 0; i < sl_len( ss ); i++ ) {
        ss[ i ] = toupper( ss[ i ] );
    }
    return ss;
}


sl_t sl_tolower( sl_t ss )
{
    for ( sl_size_t i = 0; i < sl_len( ss ); i++ ) {
        ss[ i ] = tolower( ss[ i ] );
    }
    return ss;
}


sl_t sl_read_file( char* filename )
{
    sl_t ss;

    off_t size = sl_file_size( filename );
    if ( size < 0 )
        return NULL; // GCOV_EXCL_LINE

    ss = sl_new( size + 1 );

    int fd;

    fd = open( filename, O_RDONLY );
    if ( fd == -1 )
        return NULL; // GCOV_EXCL_LINE
    read( fd, ss, size );
    ss[ size ] = 0;
    sl_len( ss ) = size;
    close( fd );

    return ss;
}


sl_t sl_write_file( sl_t ss, char* filename )
{
    int fd;

    fd = creat( filename, S_IWUSR | S_IRUSR );
    if ( fd == -1 )
        return NULL; // GCOV_EXCL_LINE
    write( fd, ss, sl_len( ss ) );
    close( fd );

    return ss;
}


void sl_print( sl_t ss )
{
    printf( "%s\n", ss );
    printf( "  len: %d\n", sl_len( ss ) );
    printf( "  res: %d\n", sl_res( ss ) );
}




/* ------------------------------------------------------------
 * Utility functions.
 * ------------------------------------------------------------ */


/**
 * Copy "src" to "dst" and return pointer to end of "dst".
 *
 * @param dst Destination str.
 * @param src Source str.
 *
 * @return Pointer to dst end.
 */
static char* sl_copy_setup( char* dst, char* src )
{
    int i = 0;
    while ( src[ i ] ) {
        dst[ i ] = src[ i ];
        i++;
    }
    return &( dst[ i ] );
}


/**
 * Return file size or (-1 on error).
 *
 * @param filename Name of file.
 *
 * @return File size.
 */
static off_t sl_file_size( const char* filename )
{
    struct stat st;

    if ( stat( filename, &st ) == 0 )
        return st.st_size;

    return -1; // GCOV_EXCL_LINE
}


/**
 * Normalize (possibly negative) SL index. Positive index is saturated
 * to SL length, and negative index is normalized.
 *
 * -1 means last char in SL, -2 second to last, etc. Index after last
 * char can only be expressed by positive indeces. E.g. for SL with
 * length of 4 the indeces are:
 *
 * Chars:     a  b  c  d  \0
 * Positive:  0  1  2  3  4
 * Negative: -4 -3 -2 -1
 *
 * @param ss  SL.
 * @param idx Index to SL.
 *
 * @return Unsigned (positive) index to SL.
 */
static sl_size_t sl_norm_idx( sl_t ss, int idx )
{
    sl_size_t ret;

    if ( idx < 0 ) {
        ret = sl_len( ss ) + idx;
    } else if ( (sl_size_t)idx > sl_len( ss ) ) {
        ret = sl_len( ss );
    } else {
        ret = idx;
    }

    return ret;
}


/**
 * Copy s2 to s1.
 *
 * @param s1   SL target.
 * @param s2   Source string.
 * @param len1 s2 length + 1
 *
 * @return SL.
 */
static sl_t sl_copy_base( sl_p s1, char* s2, sl_size_t len1 )
{
    sl_reserve( s1, len1 );
    strncpy( *s1, s2, len1 );
    sl_len( *s1 ) = len1 - 1;
    return *s1;
}


/**
 * Compare two strings.
 *
 * @param s1 String 1.
 * @param s2 String 2.
 *
 * @return Return "strcmp" result.
 */
static int sl_compare_base( const void* s1, const void* s2 )
{
    return strcmp( *(char* const*)s1, *(char* const*)s2 );
}


/**
 * Concatenate s2 to s1.
 *
 * @param s1   SL target.
 * @param s2   Source string.
 * @param len1 s2 length + 1.
 *
 * @return SL.
 */
static sl_t sl_concatenate_base( sl_p s1, char* s2, sl_size_t len1 )
{
    sl_reserve( s1, sl_len( *s1 ) + len1 );
    strncpy( sl_end( *s1 ), s2, len1 );
    sl_len( *s1 ) += len1 - 1;
    return *s1;
}


/**
 * Insert s2 of with length+1 to s1 into position pos.
 *
 * @param s1    String 1.
 * @param pos   Insert position.
 * @param s2    String 2.
 * @param len1  s2 length + 1.
 *
 * @return SL.
 */
static sl_t sl_insert_base( sl_p s1, int pos, char* s2, sl_size_t len1 )
{
    sl_size_t len = sl_len( *s1 ) + len1;
    sl_reserve( s1, len );

    len1--;

    sl_size_t posn = sl_norm_idx( *s1, pos );

    /*
     *          tail
     *         /
     * abcd....efghi
     */

    sl_size_t tail = posn + len1;

    memmove( ( *s1 ) + tail, ( *s1 ) + posn, ( sl_len( *s1 ) - posn ) );
    strncpy( ( *s1 ) + posn, s2, len1 );
    sl_len( *s1 ) += len1;

    /* Terminate change SL. */
    sl_base_p s = sl_base( *s1 );
    s->str[ s->len ] = 0;


    return *s1;
}


/**
 * Divide SL by replacing "c" with 0. Count the number of segments and
 * assign "div" to point to start of each segment.
 *
 * If size is less than 0, count only the number of segments.
 *
 * @param ss   SL.
 * @param c    Division char.
 * @param size Segment limit (size of div).
 * @param div  Storage for segments.
 *
 * @return Number of segments.
 */
static int sl_divide_base( sl_t ss, char c, int size, char** div )
{
    int   divcnt = 0;
    char *a, *b;

    a = ss;
    b = ss;

    while ( *b ) {
        if ( *b == c ) {
            if ( size >= 0 )
                *b = 0;
            if ( divcnt < size ) {
                div[ divcnt ] = a;
                b += 1;
                a = b;
            }
            divcnt++;
        }
        b++;
    }

    if ( divcnt < size && size >= 0 )
        div[ divcnt ] = a;

    return divcnt + 1;
}


/**
 * Segment SL by replacing "sc" with 0. Count the number of segments
 * and assign "div" to point to start of each segment.
 *
 * If size is less than 0, count only the number of segments.
 *
 * @param ss   SL.
 * @param c    Division char.
 * @param size Segment limit (size of div).
 * @param div  Storage for segments.
 *
 * @return Number of segments.
 */
static int sl_segment_base( sl_t ss, char* sc, int size, char** div )
{
    int   divcnt = 0;
    int   idx;
    int   len = strlen( sc );
    char *a, *b;

    a = ss;
    b = ss;

    while ( *a ) {
        idx = sl_find_index( a, sc );
        if ( idx >= 0 ) {
            b = a + idx;
            if ( size >= 0 )
                *b = 0;
            if ( divcnt < size ) {
                div[ divcnt ] = a;
            }
            b += len;
            a = b;
            divcnt++;
        } else {
            break;
        }
    }

    if ( divcnt < size && size >= 0 )
        div[ divcnt ] = a;

    return divcnt + 1;
}


/**
 * Calculate string length of u64 string conversion.
 *
 * @param u64 Integer to convert.
 *
 * @return Length.
 */
static sl_size_t sl_u64_str_len( uint64_t u64 )
{
    sl_size_t len = 0;

    do {
        u64 /= 10;
        len++;
    } while ( u64 != 0 );

    return len;
}



/**
 * Convert u64 to string.
 *
 * @param u64 Integer to convert.
 * @param str Storage for conversion.
 */
static void sl_u64_to_str( uint64_t u64, char* str )
{
    char* c;

    c = str;
    do {
        *c++ = ( u64 % 10 ) + '0';
        u64 /= 10;
    } while ( u64 != 0 );

    *c = 0;
    c--;

    /* Reverse the string. */
    char ch;
    while ( str < c ) {
        ch = *str;
        *str = *c;
        *c = ch;
        str++;
        c--;
    }
}


/**
 * Calculate string length of i64 string conversion.
 *
 * @param i64 Integer to convert.
 *
 * @return Length.
 */
static sl_size_t sl_i64_str_len( int64_t i64 )
{
    if ( i64 < 0 ) {
        return sl_u64_str_len( -i64 ) + 1;
    } else {
        return sl_u64_str_len( i64 );
    }
}


/**
 * Convert i64 to string.
 *
 * @param i64 Integer to convert.
 * @param str Storage for conversion.
 */
static void sl_i64_to_str( int64_t i64, char* str )
{
    if ( i64 < 0 ) {
        *str++ = '-';
        sl_u64_to_str( -i64, str );
    } else {
        sl_u64_to_str( i64, str );
    }
}
