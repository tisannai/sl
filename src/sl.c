/**
 * @file   sl.c
 * @author Tero Isannainen <tero.isannainen@gmail.com>
 * @date   Sat Jul  8 17:05:06 2017
 *
 * @brief  Simple to use string library.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <sys/stat.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <libgen.h>

#include "sl.h"


/* ------------------------------------------------------------
 * Utility macros
 * ------------------------------------------------------------ */

#define sl_malsize(s)  (sizeof(sl_s) + s)

#define sl_str(s)      ((char*)&(s->str[0]))
#define sl_base(s)     ((sl)((s)-(sizeof(sl_s))))
#define sl_len(s)      (((sl)((s)-(sizeof(sl_s))))->len)
#define sl_len1(s)     ((((sl)((s)-(sizeof(sl_s))))->len)+1)
#define sl_res(s)      (((sl)((s)-(sizeof(sl_s))))->res)
#define sl_end(s)      ((char*)((s)+sl_len(s)))

#define sc_len(s)    strlen(s)
#define sc_len1(s)   (strlen(s)+1)




/* ------------------------------------------------------------
 * Utility functions.
 * ------------------------------------------------------------ */


/**
 * Ensure that SL has reservation of at least size.
 *
 * @param ss   SL.
 * @param size Reservation size requirement.
 */
static void sl_ensure( slp ss, sl_size_t size )
{
  slres( ss, size );
}


/**
 * Display SL.
 *
 * @param ss SL.
 */
static void sl_prn( sls ss )
{
  printf( "%s\n", ss );
  printf( "  len: %d\n", sl_len(ss) );
  printf( "  res: %d\n", sl_res(ss) );
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
  while ( src[i] )
    {
      dst[i] = src[i];
      i++;
    }
  return &(dst[i]);
}


/**
 * Return file size or (-1 on error).
 *
 * @param filename Name of file.
 *
 * @return File size.
 */
static off_t sl_fsize( const char *filename )
{
  struct stat st;

  if ( stat( filename, &st ) == 0 )
    return st.st_size;

  return -1;
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

  if ( idx < 0 )
    {
      ret = sl_len(ss) + idx;
    }
  else if ( (sl_size_t)idx > sl_len(ss) )
    {
      ret = sl_len(ss);
    }
  else
    {
      ret = idx;
    }

  return ret;
}


/**
 * Default malloc for SL.
 *
 * @param size Allocation size.
 *
 * @return Allocation.
 */
void* sl_malloc_f( size_t size )
{
  return malloc( size );
}


/**
 * Default free for SL.
 *
 * @param ptr Allocation to free.
 */
void sl_free_f( void* ptr )
{
  free( ptr );
}


/**
 * Default realloc for SL.
 *
 * @param ptr   Existing allocation.
 * @param size  Size for re-allocation.
 *
 * @return Re-allocation.
 */
void* sl_realloc_f( void* ptr, size_t size )
{
  return realloc( ptr, size );
}


static sl_malloc_t  sl_malloc  = sl_malloc_f;
static sl_free_t    sl_free    = sl_free_f;
static sl_realloc_t sl_realloc = sl_realloc_f;



/* ------------------------------------------------------------
 * Library
 * ------------------------------------------------------------ */

void sl_cfg_alloc( sl_malloc_t  malloc,
                   sl_free_t    free,
                   sl_realloc_t realloc )
{
  sl_malloc  = malloc;
  sl_free    = free;
  sl_realloc = realloc;
}


sls slnew( sl_size_t size )
{
  sl s;
  s = (sl) sl_malloc( sl_malsize(size) );
  s->res = size;
  s->len = 0;
  s->str[0] = 0;
  return sl_str( s );
}


sls sluse( void* ss, sl_size_t size )
{
  sl s = ss;
  s->res = size - sizeof(sl_s);
  s->len = 0;
  s->str[0] = 0;
  return sl_str( s );
}


sls sldel( slp ss )
{
  sl_free( sl_base( *ss ) );
  *ss = 0;
  return NULL;
}


sls slres( slp ss, sl_size_t size )
{
  if ( sl_res( *ss ) < size )
    {
      sl s;
      s = sl_base(*ss);
      s = (sl) sl_realloc( s, sl_malsize(size) );
      s->res = size;
      *ss = sl_str(s);
    }

  return *ss;
}


sls slmin( slp ss )
{
  sl_size_t len = sl_len1(*ss);

  if ( sl_res(*ss) > len )
    {
      sl s;
      s = sl_base(*ss);
      s = (sl) sl_realloc( s, sl_malsize(len) );
      s->res = len;
      *ss = sl_str(s);
    }

  return *ss;
}


static sls slcpy_base( slp s1, char* s2, sl_size_t len1 )
{
  sl_ensure( s1, len1 );
  strncpy( *s1, s2, len1 );
  sl_len(*s1) = len1-1;
  return *s1;
}

sls slcpy  ( slp s1, sls s2 ) { return slcpy_base( s1, s2, sl_len1(s2) ); }
sls slcpy_c( slp s1, char* s2 ) { return slcpy_base( s1, s2, sc_len1(s2) ); }


sls slfil( slp ss, char c, sl_size_t cnt )
{
  ssize_t len = sl_len(*ss);
  sl_ensure( ss, len+cnt+1 );
  char* p = &((*ss)[len]);
  for ( sl_size_t i = 0; i < cnt; i++, p++ )
    *p = c;
  *p = 0;
  sl_len(*ss) += cnt;
  return *ss;
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
  sn = slnew( sl_len( ss )+1 );
  slcpy( &sn, ss );
  return sn;
}


sls slclr( sls ss )
{
  sl_len(ss) = 0;
  *ss = 0;
  return ss;
}


sls slstr_c( char* cs )
{
  sl_size_t len = sc_len1(cs);
  sls ss = slnew( len );
  strncpy( ss, cs, len );
  sl_len(ss) = len-1;
  return ss;
}


sls slsiz_c( char* cs, sl_size_t size )
{
  sl_size_t len = sc_len1(cs);
  sls ss;
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


sl slsl( sls ss )
{
  return sl_base( ss );
}


char slend( sls ss )
{
  if ( sl_len( ss ) == 0 )
    return 0;
  else
    return ss[ sl_len( ss )-1 ];
}


int slcmp( sls s1, sls s2 )
{
  return strcmp( s1, s2 );
}


int sldff( sls s1, sls s2 )
{
  if ( sl_len(s1) != sl_len(s2) )
    return 1;
  else if ( strcmp( s1, s2 ) == 0 )
    return 0;
  else
    return 1;
}


static int sl_cmp( const void * s1 , const void * s2 )
{
  return strcmp( *(char * const *) s1, * (char * const *) s2 );
}


void slsrt( sla sa, sl_size_t len )
{
  qsort( sa, len, sizeof(char*), sl_cmp );
}


static sls slcat_base( slp s1, char* s2, sl_size_t len1 )
{
  sl_ensure( s1, sl_len(*s1)+len1 );
  strncpy( sl_end(*s1), s2, len1 );
  sl_len(*s1) += len1-1;
  return *s1;
}

sls slcat  ( slp s1, sls s2 ) { return slcat_base( s1, s2, sl_len1(s2) ); }
sls slcat_c( slp s1, char* s2 ) { return slcat_base( s1, s2, sc_len1(s2) ); }


sls slpsh( slp ss, int pos, char c )
{
  pos = sl_norm_idx( *ss, pos );
  sl s = sl_base(*ss);
  sl_ensure( ss, sl_len(*ss)+1 );
  if ( (sl_size_t)pos != s->len )
    memmove( &s->str[pos+1], &s->str[pos], s->len-pos );
  s->str[pos] = c;
  s->len++;
  return *ss;
}


sls slpop( sls ss, int pos )
{
  pos = sl_norm_idx( ss, pos );
  sl s = sl_base(ss);
  if ( (sl_size_t)pos != s->len )
    {
      memmove( &s->str[pos], &s->str[pos+1], s->len-pos );
      s->len--;
    }
  return ss;
}


sls sllim( sls ss, int pos )
{
  sl s = sl_base(ss);
  s->str[pos] = 0;
  s->len = pos;
  return ss;
}


sls slcut( sls ss, int pos )
{
  sl s = sl_base(ss);
  if ( pos >= 0 )
    {
      pos = s->len-pos;
      s->str[pos] = 0;
      s->len = pos;
      return ss;
    }
  else
    {
      sl_size_t len = s->len + pos;
      pos = -pos;
      memmove( s->str, &s->str[pos], len );
      s->len = len;
      s->str[len] = 0;
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
  if ( bn < an )
    {
      int t;
      t = an;
      an = bn;
      bn = t;
    }

  sl s = sl_base(ss);
  memmove( s->str, &s->str[an], bn-an );
  s->str[ bn-an ] = 0;
  s->len = bn-an;

  return ss;
}


static sls slins_base( slp s1, int pos, char* s2, sl_size_t len1 )
{
  sl_size_t len = sl_len(*s1)+len1;
  sl_ensure( s1, len );

  len1--;

  sl_size_t posn = sl_norm_idx( *s1, pos );

  /*
   *          tail
   *         /
   * abcd....efghi
   */

  sl_size_t tail = posn + len1;

  memmove( (*s1)+tail, (*s1)+posn, (sl_len(*s1)-posn) );
  strncpy( (*s1)+posn, s2, len1 );
  sl_len(*s1) += len1;

  /* Terminate change SL. */
  sl s = sl_base(*s1);
  s->str[ s->len ] = 0;


  return *s1;
}

sls slins  ( slp s1, int pos, sls s2 ) { return slins_base( s1, pos, s2, sl_len1(s2) ); }
sls slins_c( slp s1, int pos, char* s2 ) { return slins_base( s1, pos, s2, sc_len1(s2) ); }


sls slfmt( slp ss, char* fmt, ... )
{
  sls ret;
  va_list ap;

  va_start( ap, fmt );
  ret = slvpr( ss, fmt, ap );
  va_end( ap );

  return ret;
}


sls slvpr( slp ss, char* fmt, va_list ap )
{
  va_list coap;

  /* Copy ap to coap for second va-call. */
  va_copy( coap, ap );

  int size;
  size = vsnprintf( NULL, 0, fmt, ap );

  if ( size < 0 )
    return NULL;

  size++;
  sl_ensure( ss, sl_len(*ss)+size );

  size = vsnprintf( sl_end(*ss), size, fmt, coap );
  va_end( coap );

  sl_len(*ss) += size;

  return *ss;
}


int slinv( sls ss, int pos )
{
  if ( pos > 0 )
    return -1 * (sl_len(ss) - pos);
  else
    return sl_len(ss) + pos;
}


int slfcr( sls ss, char c, sl_size_t pos )
{
  while ( pos < sl_len(ss) && ss[pos] != c )
    pos++;

  if ( pos == sl_len(ss) )
    return -1;
  else
    return pos;
}


int slfcl( sls ss, char c, sl_size_t pos )
{
  while ( pos > 0 && ss[pos] != c )
    pos--;

  if ( pos == 0 && ss[pos] != c )
    return -1;
  else
    return pos;
}


int slidx( sls s1, char* s2 )
{
  if ( s2[0] == 0 )
    return -1;

  int i1 = 0;
  int i2 = 0;
  int i;

  while ( s1[i1] )
    {
      i = i1;
      if ( s1[i] == s2[i2] )
        {
          while ( s1[i] == s2[i2] && s2[i2] )
            {
              i++;
              i2++;
            }
          if ( s2[i2] == 0 )
            return i1;
        }
      i1++;
    }

  return -1;
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
  int divcnt = 0;
  char* a, *b;

  a = ss;
  b = ss;

  while ( *b )
    {
      if ( *b == c )
        {
          if ( size >= 0 )
            *b = 0;
          if ( divcnt < size )
            {
              div[divcnt] = a;
              b += 1;
              a = b;
            }
          divcnt++;
        }
      b++;
    }

  if ( divcnt < size &&
       size >= 0 )
    div[divcnt] = a;

  return divcnt+1;
}


int sldiv( sls ss, char c, int size, char*** div )
{
  if ( size < 0 )
    {
      /* Just count size, don't replace chars. */
      return sldiv_base( ss, c, -1, NULL );
    }
  else if ( *div )
    {
      /* Use pre-allocated storage. */
      return sldiv_base( ss, c, size, *div );
    }
  else
    {
      /* Calculate size and allocate storage. */
      size = sldiv_base( ss, c, -1, NULL );
      *div = (char**) sl_malloc( size*sizeof(char*) );
      return sldiv_base( ss, c, size, *div );
    }
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
  int divcnt = 0;
  int idx;
  int len = strlen( sc );
  char* a, *b;

  a = ss;
  b = ss;

  while ( *a )
    {
      idx = slidx( a, sc );
      if ( idx >= 0 )
        {
          b = a + idx;
          if ( size >= 0 )
            *b = 0;
          if ( divcnt < size )
            {
              div[divcnt] = a;
            }
          b += len;
          a = b;
          divcnt++;
        }
      else
        {
          break;
        }
    }

  if ( divcnt < size &&
       size >= 0 )
    div[divcnt] = a;

  return divcnt+1;
}


int slseg( sls ss, char* sc, int size, char*** div )
{
  if ( size < 0 )
    {
      /* Just count size, don't replace chars. */
      return slseg_base( ss, sc, -1, NULL );
    }
  else if ( *div )
    {
      /* Use pre-allocated storage. */
      return slseg_base( ss, sc, size, *div );
    }
  else
    {
      /* Calculate size and allocate storage. */
      size = slseg_base( ss, sc, -1, NULL );
      *div = (char**) sl_malloc( size*sizeof(char*) );
      return slseg_base( ss, sc, size, *div );
    }
}


sls slglu( sla sa, sl_size_t size, char* glu )
{
  int len = 0;
  sl_size_t i;

  /* Calc sa len. */
  i = 0;
  while ( i < size )
    len += strlen( sa[i++] );

  /* Add glu len. */
  len += (size-1) * strlen( glu );

  sls ss;
  ss = slnew( len+1 );
  sl_len(ss) = len;

  /* Build result. */
  char* p = ss;
  i = 0;
  while ( i < size )
    {
      p = sl_cpy( p, sa[i] );
      if ( i < (size-1) )
        p = sl_cpy( p, glu );
      i++;
    }
  ss[sl_len(ss)] = 0;

  return ss;
}


char* sltok( sls ss, char* delim, char** pos )
{
  if ( *pos == 0 )
    {
      /* First iteration. */
      int idx;
      idx = slidx( ss, delim );
      if ( idx < 0 )
        return NULL;
      else
        {
          ss[idx] = 0;
          *pos = &(ss[idx]);
          return ss;
        }
    }
  else if ( *pos == sl_end(ss) )
    {
      /* Passed the last iteration. */
      return NULL;
    }
  else
    {
      /* Fix nulled char. */
      **pos = delim[0];
      char* p = *pos;
      p += strlen( delim );

      if ( *p == 0 )
        {
          /* ss ended with delim, so we are done. */
          *pos = sl_end(ss);
          return NULL;
        }

      /* Find next delim. */
      int idx;
      idx = slidx( p, delim );
      if ( idx < 0 )
        {
          /* Last token, mark this by: */
          *pos = sl_end(ss);
          return p;
        }
      else
        {
          p[idx] = 0;
          *pos = &(p[idx]);
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
  if ( t )
    {
      sl_len(ss) = pos-ss;
      return ss;
    }
  else
    return NULL;
}


sls sldir( sls ss )
{
  int i;

  /* Find first "/" from end to beg. */
  i = sl_len( ss );
  while ( i > 0 && ss[i] != '/' )
    i--;

  if ( i == 0 )
    {
      if ( ss[i] == '/' )
        {
          ss[1] = 0;
          sl_len(ss) = 1;
        }
      else
        {
          ss[0] = '.';
          ss[1] = 0;
          sl_len(ss) = 1;
          return ss;
        }
    }
  else
    {
      ss[i] = 0;
      sl_len(ss) = i;
    }

  return ss;
}


sls slbas( sls ss )
{
  int i;

  /* Find first "/" from end to beg. */
  i = sl_len( ss );
  while ( i > 0 && ss[i] != '/' )
    i--;

  if ( i == 0 && ss[i] != '/' )
    {
      return ss;
    }
  else
    {
      i++;
      sl_len(ss) = sl_len(ss)-i;
      memmove( ss, &(ss[i]), sl_len(ss) );
      ss[sl_len(ss)] = 0;
    }

  return ss;
}


sls slswp( sls ss, char f, char t )
{
  sl_size_t i;

  i = 0;
  while ( i < sl_len(ss) )
    {
      if ( ss[i] == f )
        ss[i] = t;
      i++;
    }

  return ss;
}


sls slmap( slp ss, char* f, char* t )
{
  /*
   * If "t" is longer than "f", loop and count how many instances of
   * "f" is found. Increase size of ss by N*t.len - N*f.len.
   *
   * Loop and find the next "f" index. Skip upto index and insert "t"
   * inplace. Continue until "f" is no more found and insert tail of
   * "ss".
   */

  sl_size_t f_len = sc_len( f );
  sl_size_t t_len = sc_len( t );

  int idx;
  char* a, *b;

  if ( t_len > f_len )
    {
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
      a = *ss;

      while ( 1 )
        {
          idx = slidx( a, f );
          if ( idx >= 0 )
            {
              cnt++;
              a += ( idx + f_len );
            }
          else
            {
              break;
            }
        }

      sl_size_t nlen;
      sl_size_t olen = sl_len(*ss);
      nlen = sl_len(*ss) - ( cnt * f_len ) + ( cnt * t_len );
      sl_ensure( ss, nlen+1 );
      sl_len(*ss) = nlen;

      /*
       * Shift original ss content to right in order to enable copying
       * chars safely from right to left.
       */
      b = &((*ss)[nlen-olen]);
      memmove( b, *ss, olen+1 );
      a = *ss;
    }
  else
    {
      /*
       * Replace XXX with YY.
       *
       * foooXXXfiiiXXXdiii
       * foooYYfiiiYYdiii
       */
      a = *ss;
      b = *ss;
    }

  while ( *b )
    {
      idx = slidx( b, f );
      if ( idx >= 0 )
        {
          strncpy( a, b, idx );
          a += idx;
          a = sl_cpy( a, t );
          b += ( idx + f_len );
        }
      else
        {
          a = sl_cpy( a, b );
          *a = 0;
          break;
        }
    }

  if ( *b == 0 )
    *a = 0;

  return *ss;
}


sls sltou( sls ss )
{
  for ( sl_size_t i = 0; i < sl_len(ss); i++ )
    {
      sl_base(ss)->str[i] = toupper( sl_base(ss)->str[i] );
    }
  return ss;
}


sls sltol( sls ss )
{
  for ( sl_size_t i = 0; i < sl_len(ss); i++ )
    {
      sl_base(ss)->str[i] = tolower( sl_base(ss)->str[i] );
    }
  return ss;
}


sls slrdf( char* filename )
{
  sls ss;

  off_t size = sl_fsize( filename );
  if ( size < 0 ) return NULL;

  ss = slnew( size+1 );

  int fd;

  fd = open( filename, O_RDONLY );
  if ( fd == -1 ) return NULL;
  read( fd, ss, size );
  ss[size] = 0;
  sl_len(ss) = size;
  close( fd );

  return ss;
}


sls slwrf( sls ss, char* filename )
{
  int fd;

  fd = creat( filename, S_IWUSR | S_IRUSR );
  if ( fd == -1 ) return NULL;
  write( fd, ss, sl_len(ss) );
  close( fd );

  return ss;
}


void slprn( sls ss )
{
  sl_prn( ss );
}
