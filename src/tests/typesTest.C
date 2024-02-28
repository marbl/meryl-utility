
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

#include "types.H"
#include "strings.H"

//
//  Test strings for test_strto()
//

char const *minu128 =  "0";
char const *maxu128 =  "340282366920938463463374607431768211455";
char const *min128  = "-170141183460469231731687303715884105728";
char const *max128  = "+170141183460469231731687303715884105727";

char const *minu64 =  "0";
char const *maxu64 =  "18446744073709551615";
char const *min64  = "-9223372036854775808";
char const *max64  = "+9223372036854775807";

char const *minu32 =  "0";
char const *maxu32 =  "4294967295";
char const *min32  = "-2147483648";
char const *max32  =  "2147483647";

char const *minu16 =  "0";
char const *maxu16 =  "65535";
char const *min16  = "-32768";
char const *max16  =  "32767";

char const *minu8 =  "0";
char const *maxu8 =  "255";
char const *min8  = "-128";
char const *max8  =  "127";

//
//  Test ranges for test_decodeRange()
//

using merylutil::ansiCode;

void
test_asciiXXXtoInt(uint8 (*XXXtoInt)(char d), bool (*isXXXdigit)(char d), char const *name, char const *type) {
  fprintf(stdout, "|--Full %s table.\n", name);
  fprintf(stdout, "|  (undefined for non-%s inputs)\n", type);
  fprintf(stdout, "|\n");
  fprintf(stdout, "|     ");
  for (uint32 jj=0; jj<16; jj++) {
    fprintf(stdout, "    .%x", jj);
  }
  fprintf(stdout, "\n");
  fprintf(stdout, "|     ");
  for (uint32 jj=0; jj<16; jj++) {
    fprintf(stdout, "    --");
  }
  fprintf(stdout, "\n");

  const char *inv = makeAnsiEscapeSequence( (const ansiCode[]) { ansiCode::Green, ansiCode::Bold, ansiCode::END } );
  const char *nml = makeAnsiEscapeSequence( (const ansiCode[]) { ansiCode::Normal,                ansiCode::END } );

  for (uint32 ii=0; ii<16; ii++) {
    fprintf(stdout, "|  %x. |", ii);
    for (uint32 jj=0; jj<16; jj++) {
      uint8 ch = jj << 4 | ii;
      if ((*isXXXdigit)(ch))
        fprintf(stdout, " %s%c=%02xh%s", inv, integerToLetter(ch), (*XXXtoInt)(ch), nml);
      else
        fprintf(stdout, " %c=%02xh", integerToLetter(ch), (*XXXtoInt)(ch));
    }
    fprintf(stdout, "\n");
  }
  fprintf(stdout, "|--\n");
  fprintf(stdout, "\n");
}


void
test_asciiBinToInt(bool display=false) {
  if (display)
    test_asciiXXXtoInt(asciiBinToInteger, isBinDigit, "asciiBinToInteger()", "binary");

  assert(asciiBinToInteger('0') == 0x00);
  assert(asciiBinToInteger('1') == 0x01);
}

void
test_asciiOctToInt(bool display=false) {
  if (display)
    test_asciiXXXtoInt(asciiOctToInteger, isOctDigit, "asciiOctToInteger()", "octal");

  assert(asciiOctToInteger('0') == 0x00);
  assert(asciiOctToInteger('1') == 0x01);
  assert(asciiOctToInteger('2') == 0x02);
  assert(asciiOctToInteger('3') == 0x03);
  assert(asciiOctToInteger('4') == 0x04);
  assert(asciiOctToInteger('5') == 0x05);
  assert(asciiOctToInteger('6') == 0x06);
  assert(asciiOctToInteger('7') == 0x07);
}

void
test_asciiDecToInt(bool display=false) {
  if (display)
    test_asciiXXXtoInt(asciiDecToInteger, isDecDigit, "asciiDecToInteger()", "decimal");

  assert(asciiDecToInteger('0') == 0x00);
  assert(asciiDecToInteger('1') == 0x01);
  assert(asciiDecToInteger('2') == 0x02);
  assert(asciiDecToInteger('3') == 0x03);
  assert(asciiDecToInteger('4') == 0x04);
  assert(asciiDecToInteger('5') == 0x05);
  assert(asciiDecToInteger('6') == 0x06);
  assert(asciiDecToInteger('7') == 0x07);
  assert(asciiDecToInteger('8') == 0x08);
  assert(asciiDecToInteger('9') == 0x09);
}

void
test_asciiHexToInt(bool display=false) {

  if (display)
    test_asciiXXXtoInt(asciiHexToInteger, isHexDigit, "asciiHexToInteger()", "hexadecimal");

  assert(asciiHexToInteger('0') == 0x00);
  assert(asciiHexToInteger('1') == 0x01);
  assert(asciiHexToInteger('2') == 0x02);
  assert(asciiHexToInteger('3') == 0x03);
  assert(asciiHexToInteger('4') == 0x04);
  assert(asciiHexToInteger('5') == 0x05);
  assert(asciiHexToInteger('6') == 0x06);
  assert(asciiHexToInteger('7') == 0x07);
  assert(asciiHexToInteger('8') == 0x08);
  assert(asciiHexToInteger('9') == 0x09);
  assert(asciiHexToInteger('A') == 0x0A);
  assert(asciiHexToInteger('B') == 0x0B);
  assert(asciiHexToInteger('C') == 0x0C);
  assert(asciiHexToInteger('D') == 0x0D);
  assert(asciiHexToInteger('E') == 0x0E);
  assert(asciiHexToInteger('F') == 0x0F);
  assert(asciiHexToInteger('a') == 0x0a);
  assert(asciiHexToInteger('b') == 0x0b);
  assert(asciiHexToInteger('c') == 0x0c);
  assert(asciiHexToInteger('d') == 0x0d);
  assert(asciiHexToInteger('e') == 0x0e);
  assert(asciiHexToInteger('f') == 0x0f);
}


bool
test_strto(bool verbose) {

  if (verbose)
    fprintf(stdout, "Testing conversion of string to unsigned integers.\n");

  assert(strtouint128(minu128) == uint128min);
  assert(strtouint128(maxu128) == uint128max);

  assert(strtouint64(minu64) == uint64min);
  assert(strtouint64(maxu64) == uint64max);

  assert(strtouint32(minu32) == uint32min);
  assert(strtouint32(maxu32) == uint32max);

  assert(strtouint16(minu16) == uint16min);
  assert(strtouint16(maxu16) == uint16max);

  assert(strtouint8(minu8) == uint8min);
  assert(strtouint8(maxu8) == uint8max);

  if (verbose)
    fprintf(stdout, "Testing conversion of string to signed integers.\n");

  assert(strtoint128(min128) == int128min);
  assert(strtoint128(max128) == int128max);

  assert(strtoint64(min64) == int64min);
  assert(strtoint64(max64) == int64max);

  assert(strtoint32(min32) == int32min);
  assert(strtoint32(max32) == int32max);

  assert(strtoint16(min16) == int16min);
  assert(strtoint16(max16) == int16max);

  assert(strtoint8(min8) == int8min);
  assert(strtoint8(max8) == int8max);

  if (verbose)
    fprintf(stdout, "Testing conversion of string to signed integers.\n");

  char  outstr[32], *outp;

  for (uint32 tt=0; tt<32; tt++)   //  Initialize string to junk.
    outstr[tt] = 100;              //

  for (uint64 tt=0; tt<999999999999999; ) {
    outp = toDec(tt, outstr);

    if (verbose)
      fprintf(stdout, "  %2u digits - %s\n", (int32)(outp - outstr), outstr);

    assert(outp[0] == 0);     //  The output pointer should point to a NUL
    assert(outp[1] == 100);   //  byte followed by uninitialized junk.
    assert(outp[2] == 100);   //

    assert(strtouint64(outstr) == tt);   //  The outstr should decode back to the input integer.

    if      (tt < 10)              assert(outstr+1   == outp);   //  Check outp is at the expected
    else if (tt < 100)             assert(outstr+2   == outp);   //  location for the number of
    else if (tt < 1000)            assert(outstr+3   == outp);   //  digits in the input.
    else if (tt < 10000)           assert(outstr+4   == outp);
    else if (tt < 100000)          assert(outstr+5   == outp);
    else if (tt < 1000000)         assert(outstr+6   == outp);
    else if (tt < 10000000)        assert(outstr+7   == outp);
    else if (tt < 100000000)       assert(outstr+8   == outp);
    else if (tt < 1000000000)      assert(outstr+9   == outp);
    else if (tt < 10000000000)     assert(outstr+10  == outp);
    else if (tt < 100000000000)    assert(outstr+11  == outp);
    else if (tt < 1000000000000)   assert(outstr+12  == outp);
    else if (tt < 10000000000000)  assert(outstr+13  == outp);
    else if (tt < 100000000000000) assert(outstr+14  == outp);

    if      (tt < 10)            tt += 1;
    else if (tt < 100)           tt += 1;
    else if (tt < 1000)          tt += 3;
    else if (tt < 10000)         tt += 7;
    else if (tt < 100000)        tt += 73;
    else if (tt < 1000000)       tt += 337;
    else if (tt < 10000000)      tt += 773;
    else if (tt < 100000000)     tt += 3733;
    else if (tt < 1000000000)    tt += 33377;
    else if (tt < 10000000000)   tt += 333337;
    else if (tt < 100000000000)  tt += 33373777;
    else if (tt < 1000000000000) tt += 333377777;
    else                         tt += 3377737777;
  }

  if (verbose)
    fprintf(stdout, "\nTests passed.\n");

  return(true);
}


#if 0
template<typename integerType>
void
test_decodeRange(void) {
  integerType   minv;
  integerType   maxv;
  char          str[1024];

  //sprintf(str, 
}
#endif


int
main(int argc, char **argv) {
  int32 arg=1;
  int32 err=0;

  omp_set_num_threads(1);

  test_asciiBinToInt(false);
  test_asciiOctToInt(false);
  test_asciiDecToInt(false);
  test_asciiHexToInt(false);
  test_strto(false);

  while (arg < argc) {
    if      (strcmp(argv[arg], "-b") == 0)   test_asciiBinToInt(true);
    else if (strcmp(argv[arg], "-o") == 0)   test_asciiOctToInt(true);
    else if (strcmp(argv[arg], "-d") == 0)   test_asciiDecToInt(true);
    else if (strcmp(argv[arg], "-h") == 0)   test_asciiHexToInt(true);
    else if (strcmp(argv[arg], "-c") == 0)   test_strto(true);
    //se if (strcmp(argv[arg], "-r") == 0)   test_decodeRange();
    else {
      fprintf(stderr, "usage: %s [-h | -c | -r]\n", argv[0]);
      fprintf(stderr, "  -b   show table of conversion of asciiBinToInteger.\n");
      fprintf(stderr, "  -o   show table of conversion of asciiOctToInteger.\n");
      fprintf(stderr, "  -d   show table of conversion of asciiDecToInteger.\n");
      fprintf(stderr, "  -h   show table of conversion of asciiHexToInteger.\n");
      fprintf(stderr, "  -c   run test converting strings to integers; takes 150 seconds.\n");
      //rintf(stderr, "  -r   run test decoding ranges.\n");
      return 1;
    }

    arg++;
  }

  return 0;
}
