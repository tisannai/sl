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
#define sl_base(s)     ((slb)((s)-(sizeof(sl_s))))
#define sl_len(s)      (((slb)((s)-(sizeof(sl_s))))->len)
#define sl_len1(s)     ((((slb)((s)-(sizeof(sl_s))))->len)+1)
#define sl_res(s)      (((slb)((s)-(sizeof(sl_s))))->res)
#define sl_end(s)      ((char*)((s)+sl_len(s)))

#define sc_len(s)      strlen(s)
#define sc_len1(s)     (strlen(s)+1)
/* clang-format on */



/* ------------------------------------------------------------
 * Utility functions.
 * ------------------------------------------------------------ */

static void sl_ensure( slp sp, sl_size_t size );
static void sl_prn( sls ss );
static char* sl_cpy( char* dst, char* src );
static off_t sl_fsize( const char* filename );
static sl_size_t sl_norm_idx( sls ss, int idx );
static sls slcpy_base( slp s1, char* s2, sl_size_t len1 );
static int sl_cmp( const void* s1, const void* s2 );
static sls slcat_base( slp s1, char* s2, sl_size_t len1 );
static sls slins_base( slp s1, int pos, char* s2, sl_size_t len1 );
static int sldiv_base( sls ss, char c, int size, char** div );
static int slseg_base( sls ss, char* sc, int size, char** div );
static sl_size_t sl_u64_str_len( uint64_t u64 );
static void sl_u64_to_str( uint64_t u64, char* str );
static sl_size_t sl_i64_str_len( int64_t i64 );
static void sl_i64_to_str( int64_t i64, char* str );


/* ------------------------------------------------------------
 * Library
 * ------------------------------------------------------------ */

sls slnew( sl_size_t size )
{
    slb s;
    s = (slb)sl_malloc( sl_malsize( size ) );
    s->res = size;
    s->len = 0;
    s->str[ 0 ] = 0;
    return sl_str( s );
}


sls sluse( void* ss, sl_size_t size )
{
    slb s = ss;
    s->res = size - sizeof( sl_s );
    s->len = 0;
    s->str[ 0 ] = 0;
    return sl_str( s );
}


sls sldel( slp sp )
{
    sl_free( sl_base( *sp ) );
    *sp = 0;
    return NULL;
}


sls slres( slp sp, sl_size_t size )
{
    if ( sl_res( *sp ) < size ) {
        slb s;
        s = sl_base( *sp );
        s = (slb)sl_realloc( s, sl_malsize( size ) );
        s->res = size;
        *sp = sl_str( s );
    }

    return *sp;
}


sls slmin( slp sp )
{
    sl_size_t len = sl_len1( *sp );

    if ( sl_res( *sp ) > len ) {
        slb s;
        s = sl_base( *sp );
        s = (slb)sl_realloc( s, sl_malsize( len ) );
        s->res = len;
        *sp = sl_str( s );
    }

    return *sp;
}


sls slcpy( slp s1, sls s2 )
{
    return slcpy_base( s1, s2, sl_len1( s2 ) );
}


sls slcpy_c( slp s1, char* s2 )
{
    return slcpy_base( s1, s2, sc_len1( s2 ) );
}


sls slfil( slp sp, char c, sl_size_t cnt )
{
    ssize_t len = sl_len( *sp );
    sl_ensure( sp, len + cnt + 1 );
    char* p = &( ( *sp )[ len ] );
    for ( sl_size_t i = 0; i < cnt; i++, p++ )
        *p = c;
    *p = 0;
    sl_len( *sp ) += cnt;
    return *sp;
}


sls slmul( slp sp, char* cs, sl_size_t cnt )
{
    ssize_t len = sl_len( *sp );
    ssize_t clen = sc_len( cs );

    sl_ensure( sp, len + cnt * clen + 1 );
    char* p = &( ( *sp )[ len ] );
    for ( sl_size_t i = 0; i < cnt; i++ ) {
        strncpy( p, cs, clen );
        *p += clen;
    }
    *p = 0;
    sl_len( *sp ) += cnt;
    return *sp;
}


sls sldup( sls ss )
{
    sls sn;
    sn = slnew( sl_res( ss ) );
    slcpy( &sn, ss );
    return sn;
}


sls slrep( sls ss )
{
    sls sn;
    sn = slnew( sl_len( ss ) + 1 );
    slcpy( &sn, ss );
    return sn;
}


sls slclr( sls ss )
{
    sl_len( ss ) = 0;
    *ss = 0;
    return ss;
}


sls slstr_c( char* cs )
{
    sl_size_t len = sc_len1( cs );
    sls       ss = slnew( len );
    strncpy( ss, cs, len );
    sl_len( ss ) = len - 1;
    return ss;
}


sls slsiz_c( char* cs, sl_size_t size )
{
    sl_size_t len = sc_len1( cs );
    sls       ss;
    if ( size > len )
        ss = slnew( size );
    else
        ss = slnew( len );
    slcpy_c( &ss, cs );
    return ss;
}


sl_size_t sllen( sls ss )
{
    return sl_len( ss );
}


sl_size_t slall( sls ss )
{
    return sl_res( ss );
}


slb slsl( sls ss )
{
    return sl_base( ss );
}


char slend( sls ss )
{
    if ( sl_len( ss ) == 0 )
        return 0;
    else
        return ss[ sl_len( ss ) - 1 ];
}


int slcmp( sls s1, sls s2 )
{
    return strcmp( s1, s2 );
}


int sldff( sls s1, sls s2 )
{
    if ( sl_len( s1 ) != sl_len( s2 ) )
        return 1;
    else if ( strcmp( s1, s2 ) == 0 )
        return 0;
    else
        return 1;
}


void slsrt( sla sa, sl_size_t len )
{
    qsort( sa, len, sizeof( char* ), sl_cmp );
}


sls slcat( slp s1, sls s2 )
{
    return slcat_base( s1, s2, sl_len1( s2 ) );
}


sls slcat_c( slp s1, char* s2 )
{
    return slcat_base( s1, s2, sc_len1( s2 ) );
}


sls slpsh( slp sp, int pos, char c )
{
    pos = sl_norm_idx( *sp, pos );
    slb s = sl_base( *sp );
    sl_ensure( sp, sl_len( *sp ) + 1 );
    if ( (sl_size_t)pos != s->len )
        memmove( &s->str[ pos + 1 ], &s->str[ pos ], s->len - pos );
    s->str[ pos ] = c;
    s->len++;
    s->str[ s->len ] = 0;
    return *sp;
}


sls slpop( sls ss, int pos )
{
    pos = sl_norm_idx( ss, pos );
    slb s = sl_base( ss );
    if ( (sl_size_t)pos != s->len ) {
        memmove( &s->str[ pos ], &s->str[ pos + 1 ], s->len - pos );
        s->len--;
    }
    return ss;
}


sls sllim( sls ss, int pos )
{
    slb s = sl_base( ss );
    s->str[ pos ] = 0;
    s->len = pos;
    return ss;
}


sls slcut( sls ss, int cnt )
{
    int pos;
    slb s = sl_base( ss );
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


sls slsel( sls ss, int a, int b )
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

    slb s = sl_base( ss );
    memmove( s->str, &s->str[ an ], bn - an );
    s->str[ bn - an ] = 0;
    s->len = bn - an;

    return ss;
}


sls slins( slp s1, int pos, sls s2 )
{
    return slins_base( s1, pos, s2, sl_len1( s2 ) );
}


sls slins_c( slp s1, int pos, char* s2 )
{
    return slins_base( s1, pos, s2, sc_len1( s2 ) );
}


sls slfmt( slp sp, char* fmt, ... )
{
    sls     ret;
    va_list ap;

    va_start( ap, fmt );
    ret = slvpr( sp, fmt, ap );
    va_end( ap );

    return ret;
}


sls slvpr( slp sp, char* fmt, va_list ap )
{
    va_list coap;

    /* Copy ap to coap for second va-call. */
    va_copy( coap, ap );

    int size;
    size = vsnprintf( NULL, 0, fmt, ap );

    if ( size < 0 )
        return NULL;

    /* CHECK THIS. */
    size++;
    sl_ensure( sp, sl_len( *sp ) + size );

    size = vsnprintf( sl_end( *sp ), size, fmt, coap );
    va_end( coap );

    sl_len( *sp ) += size;

    return *sp;
}


sls slfmq( slp sp, char* fmt, ... )
{
    sls     ret;
    va_list ap;

    va_start( ap, fmt );
    ret = slvpq( sp, fmt, ap );
    va_end( ap );

    return ret;
}


sls slvpq( slp sp, char* fmt, va_list ap )
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

    sl_ensure( sp, sl_len1( *sp ) + size );


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


int slinv( sls ss, int pos )
{
    if ( pos > 0 )
        return -1 * ( sl_len( ss ) - pos );
    else
        return sl_len( ss ) + pos;
}


int slfcr( sls ss, char c, sl_size_t pos )
{
    while ( pos < sl_len( ss ) && ss[ pos ] != c )
        pos++;

    if ( pos == sl_len( ss ) )
        return -1;
    else
        return pos;
}


int slfcl( sls ss, char c, sl_size_t pos )
{
    while ( pos > 0 && ss[ pos ] != c )
        pos--;

    if ( pos == 0 && ss[ pos ] != c )
        return -1;
    else
        return pos;
}


int slidx( sls s1, char* s2 )
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


int sldiv( sls ss, char c, int size, char*** div )
{
    if ( size < 0 ) {
        /* Just count size, don't replace chars. */
        return sldiv_base( ss, c, -1, NULL );
    } else if ( *div ) {
        /* Use pre-allocated storage. */
        return sldiv_base( ss, c, size, *div );
    } else {
        /* Calculate size and allocate storage. */
        size = sldiv_base( ss, c, -1, NULL );
        *div = (char**)sl_malloc( size * sizeof( char* ) );
        return sldiv_base( ss, c, size, *div );
    }
}


int slseg( sls ss, char* sc, int size, char*** div )
{
    if ( size < 0 ) {
        /* Just count size, don't replace chars. */
        return slseg_base( ss, sc, -1, NULL );
    } else if ( *div ) {
        /* Use pre-allocated storage. */
        return slseg_base( ss, sc, size, *div );
    } else {
        /* Calculate size and allocate storage. */
        size = slseg_base( ss, sc, -1, NULL );
        *div = (char**)sl_malloc( size * sizeof( char* ) );
        return slseg_base( ss, sc, size, *div );
    }
}


sls slglu( sla sa, sl_size_t size, char* glu )
{
    int       len = 0;
    sl_size_t i;

    /* Calc sa len. */
    i = 0;
    while ( i < size )
        len += strlen( sa[ i++ ] );

    /* Add glu len. */
    len += ( size - 1 ) * strlen( glu );

    sls ss;
    ss = slnew( len + 1 );
    sl_len( ss ) = len;

    /* Build result. */
    char* p = ss;
    i = 0;
    while ( i < size ) {
        p = sl_cpy( p, sa[ i ] );
        if ( i < ( size - 1 ) )
            p = sl_cpy( p, glu );
        i++;
    }
    ss[ sl_len( ss ) ] = 0;

    return ss;
}


char* sltok( sls ss, char* delim, char** pos )
{
    if ( *pos == 0 ) {
        /* First iteration. */
        int idx;
        idx = slidx( ss, delim );
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
        idx = slidx( p, delim );
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


sls slext( sls ss, char* ext )
{
    char* pos;
    char* t;

    pos = NULL;
    t = sltok( ss, ext, &pos );
    if ( t ) {
        sl_len( ss ) = pos - ss;
        return ss;
    } else
        return NULL;
}


sls sldir( sls ss )
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


sls slbas( sls ss )
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


sls slswp( sls ss, char f, char t )
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


sls slmap( slp sp, char* f, char* t )
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
            idx = slidx( a, f );
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
        sl_ensure( sp, nlen + 1 );
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
        idx = slidx( b, f );
        if ( idx >= 0 ) {
            strncpy( a, b, idx );
            a += idx;
            a = sl_cpy( a, t );
            b += ( idx + f_len );
        } else {
            a = sl_cpy( a, b );
            *a = 0;
            break;
        }
    }

    if ( *b == 0 )
        *a = 0;

    return *sp;
}


sls slcap( sls ss )
{
    if ( sl_len( ss ) > 0 )
        ss[ 0 ] = toupper( ss[ 0 ] );

    return ss;
}


sls sltou( sls ss )
{
    for ( sl_size_t i = 0; i < sl_len( ss ); i++ ) {
        ss[ i ] = toupper( ss[ i ] );
    }
    return ss;
}


sls sltol( sls ss )
{
    for ( sl_size_t i = 0; i < sl_len( ss ); i++ ) {
        ss[ i ] = tolower( ss[ i ] );
    }
    return ss;
}


sls slrdf( char* filename )
{
    sls ss;

    off_t size = sl_fsize( filename );
    if ( size < 0 )
        return NULL;

    ss = slnew( size + 1 );

    int fd;

    fd = open( filename, O_RDONLY );
    if ( fd == -1 )
        return NULL;
    read( fd, ss, size );
    ss[ size ] = 0;
    sl_len( ss ) = size;
    close( fd );

    return ss;
}


sls slwrf( sls ss, char* filename )
{
    int fd;

    fd = creat( filename, S_IWUSR | S_IRUSR );
    if ( fd == -1 )
        return NULL;
    write( fd, ss, sl_len( ss ) );
    close( fd );

    return ss;
}


void slprn( sls ss )
{
    sl_prn( ss );
}




/* ------------------------------------------------------------
 * Utility functions.
 * ------------------------------------------------------------ */


/**
 * Ensure that SL has reservation of at least size.
 *
 * @param ss   SL.
 * @param size Reservation size requirement.
 */
static void sl_ensure( slp sp, sl_size_t size )
{
    slres( sp, size );
}


/**
 * Display SL.
 *
 * @param ss SL.
 */
static void sl_prn( sls ss )
{
    printf( "%s\n", ss );
    printf( "  len: %d\n", sl_len( ss ) );
    printf( "  res: %d\n", sl_res( ss ) );
}


/**
 * Copy "src" to "dst" and return pointer to end of "dst".
 *
 * @param dst Destination str.
 * @param src Source str.
 *
 * @return Pointer to dst end.
 */
static char* sl_cpy( char* dst, char* src )
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
static off_t sl_fsize( const char* filename )
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
static sl_size_t sl_norm_idx( sls ss, int idx )
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
static sls slcpy_base( slp s1, char* s2, sl_size_t len1 )
{
    sl_ensure( s1, len1 );
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
static int sl_cmp( const void* s1, const void* s2 )
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
static sls slcat_base( slp s1, char* s2, sl_size_t len1 )
{
    sl_ensure( s1, sl_len( *s1 ) + len1 );
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
static sls slins_base( slp s1, int pos, char* s2, sl_size_t len1 )
{
    sl_size_t len = sl_len( *s1 ) + len1;
    sl_ensure( s1, len );

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
    slb s = sl_base( *s1 );
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
static int sldiv_base( sls ss, char c, int size, char** div )
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
static int slseg_base( sls ss, char* sc, int size, char** div )
{
    int   divcnt = 0;
    int   idx;
    int   len = strlen( sc );
    char *a, *b;

    a = ss;
    b = ss;

    while ( *a ) {
        idx = slidx( a, sc );
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
