#include "unity.h"
#include "sl.h"
#include <string.h>
#include <unistd.h>

void test_basics( void )
{
  sls s, s2;
  char* t1 = "text1";

  sl_cfg_alloc( sl_malloc_f, sl_free_f, sl_realloc_f );

  s = slnew( 128 );

  slcpy_c( &s, t1 );
  TEST_ASSERT_TRUE( !strcmp( s, t1 ) );
  TEST_ASSERT( slall(s) == 128 );
  TEST_ASSERT( sllen(s) == 5 );

  slmin( &s );
  TEST_ASSERT( slall(s) == 6 );
  TEST_ASSERT( sllen(s) == 5 );

  slcpy( &s, s );
  TEST_ASSERT( slall(s) == 6 );
  TEST_ASSERT( sllen(s) == 5 );

  slcat( &s, s );
  TEST_ASSERT( slall(s) == 11 );
  TEST_ASSERT( sllen(s) == 10 );

  s2 = sldup( s );
  sldel( &s );
  sldel( &s2 );

  s2 = slsiz_c( t1, 2 );
  TEST_ASSERT_TRUE( !strcmp( s2, "text1" ) );
  TEST_ASSERT( slall(s2) == 6 );
  TEST_ASSERT( sllen(s2) == 5 );
  sl s2sl;
  s2sl = slsl( s2 );
  TEST_ASSERT( !strcmp( s2sl->str, "text1" ) );
  TEST_ASSERT( s2sl->res == 6 );
  TEST_ASSERT( s2sl->len == 5 );
  sldel( &s2 );

  s = sluse( sl_malloc_f( 1024 ), 1024 );
  slcpy_c( &s, t1 );
  slcat( &s, s );
  slcat_c( &s, t1 );
  TEST_ASSERT_TRUE( !strcmp( s, "text1text1text1" ) );

  sldel( &s );
}


void test_sizing( void )
{
  sls s;

  s = slnew( 128 );

  slres( &s, 64 );
  TEST_ASSERT( slall(s) == 128 );

  slres( &s, 128 );
  TEST_ASSERT( slall(s) == 128 );

  slres( &s, 129 );
  TEST_ASSERT( slall(s) == 129 );

  slmin( &s );
  TEST_ASSERT( slall(s) == 1 );

  slres( &s, 64 );
  TEST_ASSERT( slall(s) == 64 );

  sldel( &s );
}


void test_content( void )
{
  sls s;

  char* t1 = "text1";

  s = slstr_c( t1 );
  TEST_ASSERT_TRUE( !strcmp( s, t1 ) );
  TEST_ASSERT( slall(s) == 6 );
  TEST_ASSERT( sllen(s) == 5 );
  sldel( &s );

  s = slsiz_c( t1, 12 );
  TEST_ASSERT( slall(s) == 12 );
  TEST_ASSERT( sllen(s) == 5 );

  slcat( &s, s );
  TEST_ASSERT( slall(s) == 12 );
  TEST_ASSERT( sllen(s) == 10 );

  slcat_c( &s, t1 );
  TEST_ASSERT( slall(s) == 16 );
  TEST_ASSERT( sllen(s) == 15 );

  slcut( s, 2 );
  TEST_ASSERT_TRUE( !strcmp( s, "text1text1tex" ) );
  TEST_ASSERT( slall(s) == 16 );
  TEST_ASSERT( sllen(s) == 13 );

  slcut( s, -2 );
  TEST_ASSERT_TRUE( !strcmp( s, "xt1text1tex" ) );
  TEST_ASSERT( slall(s) == 16 );
  TEST_ASSERT( sllen(s) == 11 );

  sls s2;
  s2 = slsel( s, 1, -2 );
  TEST_ASSERT_TRUE( !strcmp( s2, "t1text1te" ) );
  TEST_ASSERT( slall(s2) == 10 );
  TEST_ASSERT( sllen(s2) == 9 );

  s2 = slsel( s, -2, 1 );
  TEST_ASSERT_TRUE( !strcmp( s2, "t1text1te" ) );
  TEST_ASSERT( slall(s2) == 10 );
  TEST_ASSERT( sllen(s2) == 9 );

  slcpy_c( &s, t1 );
  slfmt( &s, "__%s_", t1 );
  TEST_ASSERT_TRUE( !strcmp( s, "text1__text1_" ) );
  TEST_ASSERT( slall(s) == 16 );
  TEST_ASSERT( sllen(s) == 13 );

  slclr( s );
  slfmt( &s, "__%s_", t1 );
  TEST_ASSERT_TRUE( !strcmp( s, "__text1_" ) );
  TEST_ASSERT( slall(s) == 16 );
  TEST_ASSERT( sllen(s) == 8 );

  slfil( &s, 'a', 10 );
  TEST_ASSERT_TRUE( !strcmp( s, "__text1_aaaaaaaaaa" ) );
  TEST_ASSERT( slall(s) == 19 );
  TEST_ASSERT( sllen(s) == 18 );

  slclr( s );
  slfil( &s, 'a', 10 );
  TEST_ASSERT_TRUE( !strcmp( s, "aaaaaaaaaa" ) );
  TEST_ASSERT( slall(s) == 19 );
  TEST_ASSERT( sllen(s) == 10 );

  sldel( &s );
  sldel( &s2 );
}


void test_insert( void )
{
  sls s;
  char* t1 = "text1";

  s = slstr_c( t1 );

  slins_c( &s, 0, t1 );
  TEST_ASSERT_TRUE( !strcmp( s, "text1text1" ) );
  TEST_ASSERT( slall(s) == 11 );
  TEST_ASSERT( sllen(s) == 10 );

  slins( &s, 128, s );
  TEST_ASSERT_TRUE( !strcmp( s, "text1text1text1text1" ) );
  TEST_ASSERT( slall(s) == 21 );
  TEST_ASSERT( sllen(s) == 20 );

}


void test_examine( void )
{
  sls s;
  int pos;

  char* t1 = "abcdefghijkl";

  s = slstr_c( t1 );

  pos = slidx( s, "a" );
  TEST_ASSERT( pos == 0 );

  pos = slidx( s, "b" );
  TEST_ASSERT( pos == 1 );

  pos = slidx( s, "k" );
  TEST_ASSERT( pos == 10 );

  pos = slidx( s, "l" );
  TEST_ASSERT( pos == 11 );

  pos = slidx( s, "ab" );
  TEST_ASSERT( pos == 0 );

  pos = slidx( s, "kl" );
  TEST_ASSERT( pos == 10 );

  /* Invalid search. */
  pos = slidx( s, "" );
  TEST_ASSERT( pos == -1 );

}


void test_pieces( void )
{
  sls s, s2;

  char** pcs;
  int cnt;

  s = slstr_c( "XYabcXYabcXY" );
  TEST_ASSERT( slall(s) == 13 );
  TEST_ASSERT( sllen(s) == 12 );

  cnt = sldiv( s, 'X', -1, NULL );
  TEST_ASSERT( cnt == 4 );

  cnt = sldiv( s, 'Y', -1, NULL );
  TEST_ASSERT( cnt == 4 );

  cnt = sldiv( s, 'a', -1, NULL );
  TEST_ASSERT( cnt == 3 );

  /* ------------------------------------------------------------
   * sldiv
   */
  pcs = NULL;
  cnt = sldiv( s, 'X', 0, &pcs );
  TEST_ASSERT_TRUE( !strcmp( pcs[0], "" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[1], "Yabc" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[2], "Yabc" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[3], "Y" ) );
  s2 = slglu( pcs, cnt, "H" );
  TEST_ASSERT_TRUE( !strcmp( s2, "HYabcHYabcHY" ) );
  TEST_ASSERT( slall(s2) == 13 );
  TEST_ASSERT( sllen(s2) == 12 );
  slswp( s, 0, 'X' );
  sl_free_f( pcs );

  pcs = NULL;
  cnt = sldiv( s, 'Y', 0, &pcs );
  TEST_ASSERT_TRUE( !strcmp( pcs[0], "X" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[1], "abcX" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[2], "abcX" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[3], "" ) );
  s2 = slglu( pcs, cnt, "H" );
  TEST_ASSERT_TRUE( !strcmp( s2, "XHabcXHabcXH" ) );
  TEST_ASSERT( slall(s2) == 13 );
  TEST_ASSERT( sllen(s2) == 12 );
  slswp( s, 0, 'Y' );
  sl_free_f( pcs );

  pcs = NULL;
  cnt = sldiv( s, 'a', 0, &pcs );
  TEST_ASSERT_TRUE( !strcmp( pcs[0], "XY" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[1], "bcXY" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[2], "bcXY" ) );
  slswp( s, 0, 'a' );
  sl_free_f( pcs );

  {
    char* spc[ cnt ];
    char** help = spc;
    cnt = sldiv( s, 'a', -1, NULL );
    sldiv( s, 'a', cnt, &help );
    TEST_ASSERT_TRUE( !strcmp( spc[0], "XY" ) );
    TEST_ASSERT_TRUE( !strcmp( spc[1], "bcXY" ) );
    TEST_ASSERT_TRUE( !strcmp( spc[2], "bcXY" ) );
    s2 = slglu( spc, cnt, "A" );
    TEST_ASSERT_TRUE( !strcmp( s2, "XYAbcXYAbcXY" ) );
    TEST_ASSERT( slall(s2) == 13 );
    TEST_ASSERT( sllen(s2) == 12 );
    slswp( s, 0, 'a' );
  }


  /* ------------------------------------------------------------
   * slseg
   */
  pcs = NULL;
  cnt = slseg( s, "XY", 0, &pcs );
  TEST_ASSERT_TRUE( !strcmp( pcs[0], "" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[1], "abc" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[2], "abc" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[3], "" ) );
  s2 = slglu( pcs, cnt, "H" );
  TEST_ASSERT_TRUE( !strcmp( s2, "HabcHabcH" ) );
  TEST_ASSERT( slall(s2) == 10 );
  TEST_ASSERT( sllen(s2) == 9 );
  slswp( s, 0, 'X' );
  sl_free_f( pcs );

  pcs = NULL;
  cnt = slseg( s, "a", 0, &pcs );
  TEST_ASSERT_TRUE( !strcmp( pcs[0], "XY" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[1], "bcXY" ) );
  TEST_ASSERT_TRUE( !strcmp( pcs[2], "bcXY" ) );
  slswp( s, 0, 'a' );
  sl_free_f( pcs );

  {
    char* spc[ cnt ];
    char** help = spc;
    cnt = slseg( s, "a", -1, NULL );
    slseg( s, "a", cnt, &help );
    TEST_ASSERT_TRUE( !strcmp( spc[0], "XY" ) );
    TEST_ASSERT_TRUE( !strcmp( spc[1], "bcXY" ) );
    TEST_ASSERT_TRUE( !strcmp( spc[2], "bcXY" ) );
    s2 = slglu( spc, cnt, "A" );
    TEST_ASSERT_TRUE( !strcmp( s2, "XYAbcXYAbcXY" ) );
    TEST_ASSERT( slall(s2) == 13 );
    TEST_ASSERT( sllen(s2) == 12 );
    slswp( s, 0, 'a' );
  }

  sl_free_f( pcs );

}


void test_tok( void )
{
  sls s;
  char* t, *pos, *delim = "XY";

  s = slstr_c( "XYabXYabcXYc" );
  pos = NULL;
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "" ) );
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "ab" ) );
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "abc" ) );
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "c" ) );

  t = sltok( s, delim, &pos );
  TEST_ASSERT( t == NULL );

  sldel( &s );

  s = slstr_c( "XYabXYabcXYcXY" );
  pos = NULL;
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "" ) );
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "ab" ) );
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "abc" ) );
  t = sltok( s, delim, &pos );
  TEST_ASSERT_TRUE( !strcmp( t, "c" ) );

  t = sltok( s, delim, &pos );
  TEST_ASSERT( t == NULL );

  sldel( &s );

  s = slstr_c( "XYabXYabcXYcXY" );
  pos = NULL;
  t = sltok( s, "foo", &pos );
  TEST_ASSERT( t == NULL );

  sldel( &s );
}


void test_map( void )
{
  sls s;

  s = slstr_c( "XYabcXYabcXY" );
  TEST_ASSERT( slall(s) == 13 );
  TEST_ASSERT( sllen(s) == 12 );

  slmap( &s, "XY", "GIG" );
  TEST_ASSERT_TRUE( !strcmp( s, "GIGabcGIGabcGIG" ) );
  sldel( &s );

  s = slstr_c( "XYabcXYabc" );
  TEST_ASSERT( slall(s) == 11 );
  TEST_ASSERT( sllen(s) == 10 );

  slmap( &s, "XY", "GIG" );
  TEST_ASSERT_TRUE( !strcmp( s, "GIGabcGIGabc" ) );
  sldel( &s );

  s = slstr_c( "XYabcXYabc" );
  TEST_ASSERT( slall(s) == 11 );
  TEST_ASSERT( sllen(s) == 10 );

  slmap( &s, "XY", "GG" );
  TEST_ASSERT_TRUE( !strcmp( s, "GGabcGGabc" ) );
  sldel( &s );
}


void test_file( void )
{
  char* filetext = "\
line1\n\
line2\n\
line3\n\
line4\n\
line5\n\
";

  sls s, s2;
  s = slstr_c( filetext );
  slwrf( s, "test_file.txt" );
  s2 = slrdf( "test_file.txt" );
  TEST_ASSERT_TRUE( !strcmp( s2, filetext ) );

  slprn( s );
  sldel( &s );
  sldel( &s2 );
  TEST_ASSERT( s == NULL );
  TEST_ASSERT( s2 == NULL );
}


void test_path( void )
{
  char* path1 = "/foo/bar/dii.txt";
  char* path2 = "./foo/bar/dii.txt";
  char* path3 = "/foo";
  char* path4 = "./foo";
  char* path5 = "dii.txt";

  sls s;

  s = slstr_c( path1 );
  sldir( s );
  TEST_ASSERT_TRUE( !strcmp( s, "/foo/bar" ) );
  TEST_ASSERT( sllen(s) == 8 );
  sldel( &s );

  s = slstr_c( path2 );
  sldir( s );
  TEST_ASSERT_TRUE( !strcmp( s, "./foo/bar" ) );
  TEST_ASSERT( sllen(s) == 9 );
  sldel( &s );

  s = slstr_c( path3 );
  sldir( s );
  TEST_ASSERT_TRUE( !strcmp( s, "/" ) );
  TEST_ASSERT( sllen(s) == 1 );
  sldel( &s );

  s = slstr_c( path4 );
  sldir( s );
  TEST_ASSERT_TRUE( !strcmp( s, "." ) );
  TEST_ASSERT( sllen(s) == 1 );
  sldel( &s );

  s = slstr_c( path5 );
  sldir( s );
  TEST_ASSERT_TRUE( !strcmp( s, "." ) );
  TEST_ASSERT( sllen(s) == 1 );
  sldel( &s );

  s = slstr_c( path1 );
  slbas( s );
  TEST_ASSERT_TRUE( !strcmp( s, "dii.txt" ) );
  TEST_ASSERT( sllen(s) == 7 );
  sldel( &s );

  s = slstr_c( path2 );
  slbas( s );
  TEST_ASSERT_TRUE( !strcmp( s, "dii.txt" ) );
  TEST_ASSERT( sllen(s) == 7 );
  sldel( &s );

  s = slstr_c( path3 );
  slbas( s );
  TEST_ASSERT_TRUE( !strcmp( s, "foo" ) );
  TEST_ASSERT( sllen(s) == 3 );
  sldel( &s );

  s = slstr_c( path4 );
  slbas( s );
  TEST_ASSERT_TRUE( !strcmp( s, "foo" ) );
  TEST_ASSERT( sllen(s) == 3 );
  sldel( &s );

  s = slstr_c( path5 );
  slbas( s );
  TEST_ASSERT_TRUE( !strcmp( s, "dii.txt" ) );
  TEST_ASSERT( sllen(s) == 7 );
  sldel( &s );

}
