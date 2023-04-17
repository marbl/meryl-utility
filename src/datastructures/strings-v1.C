
/******************************************************************************
 *
 *  This file is part of meryl-utility, a collection of miscellaneous code
 *  used by Meryl, Canu and others.
 *
 *  This software is based on:
 *    'Canu' v2.0              (https://github.com/marbl/canu)
 *  which is based on:
 *    'Celera Assembler' r4587 (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' r1994 (http://kmer.sourceforge.net)
 *
 *  Except as indicated otherwise, this is a 'United States Government Work',
 *  and is released in the public domain.
 *
 *  File 'README.licenses' in the root directory of this distribution
 *  contains full conditions and disclaimers.
 */

#include "strings.H"

namespace merylutil::inline strings::inline v1 {

////////////////////////////////////////////////////////////
//
//  Strip whitespace from the end of a line.
//
void
chomp(char *S) {
  char *t = S;

  while (*t != 0)
    t++;

  t--;

  while ((t >= S) && (isWhiteSpace(*t) == true))
    *t-- = 0;
}



////////////////////////////////////////////////////////////
//
//  In-place removes whitespace from either end of a string.
//
//  The non-whitespace portion of the string is shifted to the front of the
//  storage and unused space is filled with NUL bytes.  Any bytes after the
//  first NUL byte are not modified.
//
//  Using '@' to represent NUL bytes:
//
//    input:   s = '   hello world    @'
//    output:  s = 'hello world@@@@@@@@'
//
//    input:   s = '   hello@world@    '
//    output:  s = 'hello@@@@world@    '
//
//    input:   s = 'hello@kq4Bq4tab9q5B'
//    output:  s = 'hello@kq4Bq4tab9q5B'
//
//  The pointer to s is returned from the function.
//
char *
trimString(char *s, bool bgn, bool end) {
  int64 i=0, b=0, e=0;

  while (s[e] != 0)                          //  Find the end of the string.
    e++;                                     //  We later expect 'e' to be the
  e--;                                       //  last letter of the string.

  if (end)                                   //  Trim spaces at the end.
    while ((e >= 0) && (s[e] == ' '))        //
      s[e--] = 0;                            //

  if (bgn)                                   //  Trim spaces from the start.
    while ((b < e) && (s[b] == ' '))         //
      b++;                                   //

  while (b <= e)                             //  Shift the string to actually
    s[i++] = s[b++];                         //  remove the spaces at the start.

  while (i <= e)                             //  Terminate the new string and
    s[i++] = 0;                              //  erase left over crud.

  return s;
}



}  //  merylutil::strings::v1
