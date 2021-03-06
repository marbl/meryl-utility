
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

#include "bits.H"
#include "strings.H"
#include "mt19937ar.H"

char          b1[65];
char          b2[65];
char          b3[65];


void
testMasks(void) {
  uint128  m128;
  uint64   m64;
  uint32   m32;
  uint16   m16;
  uint8    m8;

  for (uint32 ii=0; ii<=128; ii++) {
    fprintf(stderr, "%3d: %s %s  %s %s  %s %s  %s %s  %s %s\n", ii,
            toHex(buildLowBitMask<uint128>(ii)), toHex(buildHighBitMask<uint128>(ii)),
            toHex(buildLowBitMask<uint64> (ii)), toHex(buildHighBitMask<uint64> (ii)),
            toHex(buildLowBitMask<uint32> (ii)), toHex(buildHighBitMask<uint32> (ii)),
            toHex(buildLowBitMask<uint16> (ii)), toHex(buildHighBitMask<uint16> (ii)),
            toHex(buildLowBitMask<uint8>  (ii)), toHex(buildHighBitMask<uint8>  (ii)));
  }
}


void
testLogBaseTwo(void) {
  uint64  val = 0;

  for (uint32 ii=0; ii<64; ii++) {
    assert(ii == countNumberOfBits64(val));

    val <<= 1;
    //val++;

    if (val == 0)
      val = 1;
  }
}


void
testSaveClear(void) {
  uint64  bb = 0xffffffffffffffffllu;

  for (uint32 ii=0; ii<65; ii++)
    fprintf(stderr, "%2u clearLeft %s clearRight %s\n",
            ii,
            displayWord(clearLeftBits (bb, ii), b1),
            displayWord(clearRightBits(bb, ii), b2));

  for (uint32 ii=0; ii<65; ii++)
    fprintf(stderr, "%2u  saveLeft %s  saveRight %s\n",
            ii,
            displayWord(saveLeftBits (bb, ii), b1),
            displayWord(saveRightBits(bb, ii), b2));

  for (uint32 ii=0; ii<65; ii++)
    fprintf(stderr, "%2u  saveMid  %s clearMid   %s\n",
            ii,
            displayWord(saveMiddleBits (bb, ii, 10), b1),
            displayWord(clearMiddleBits(bb, ii, 10), b2));

  for (uint32 ii=0; ii<65; ii++)
    fprintf(stderr, "%2u  saveMid  %s clearMid   %s\n",
            ii,
            displayWord(saveMiddleBits (bb, 10, ii), b1),
            displayWord(clearMiddleBits(bb, 10, ii), b2));
}



void
testBitArray(uint64 maxLength) {
  bitArray  *ba = new bitArray(maxLength);

  for (uint32 delta=1; delta<3; delta++) {
    fprintf(stderr, "testBitArray()-- delta %u\n", delta);

    for (uint64 xx=0; xx<maxLength; xx += delta) {
      ba->setBit(xx, 1);
    }

    for (uint64 xx=0; xx<maxLength; xx++) {
      if ((xx % delta) == 0) {
        //fprintf(stderr, "TOGGLE pos %3" F_U64P " 1->0  ", xx);
        assert(ba->getBit(xx) == 1);
        ba->setBit(xx, 0);
        assert(ba->getBit(xx) == 0);
      } else {
        //fprintf(stderr, "TOGGLE pos %3" F_U64P " 0->1  ", xx);
        assert(ba->getBit(xx) == 0);
        ba->setBit(xx, 1);
        assert(ba->getBit(xx) == 1);
      }
    }

    ba->clear();
  }

  delete ba;
}



void
testWordArray(uint64 wordSize) {
  wordArray  *wa = new wordArray(wordSize, 8 * 64, false);

  for (uint32 ii=0; ii<1000; ii++)
    wa->set(ii, 0xffffffff);

  for (uint32 ii=0; ii<1000; ii++)
    wa->set(ii, ii);

  wa->show();

  for (uint32 ii=0; ii<1000; ii++)
    assert(wa->get(ii) == (ii & buildLowBitMask<uint64>(wordSize)));

  fprintf(stderr, "Passed!\n");

  delete wa;
}



void
testUnary(uint32 testSize) {
  uint32      maxN   = 10000000;
  uint32     *random = new uint32 [maxN];
  mtRandom    mt;

  fprintf(stderr, "Creating.\n");

  uint64  total = 0;

  for (uint32 ii=0; ii<maxN; ii++) {
    random[ii]  = mt.mtRandom32() % testSize;
    total      += random[ii] + 1;
  }

  fprintf(stderr, "Created " F_U64 " Mbits, " F_U64 " MB needed.\n", total >> 20, (total + maxN * sizeof(uint32) * 8) >> 23);

  total = 0;

  fprintf(stderr, "Setting.\n");

  stuffedBits *bits = new stuffedBits;

  for (uint32 ii=0; ii<maxN; ii++) {
    bits->setUnary(random[ii]);
    total += random[ii] + 1;

    assert(bits->getPosition() == total);
  }

  fprintf(stderr, "Testing.\n");

  bits->setPosition(0);

  for (uint32 ii=0; ii<maxN; ii++)
    assert(random[ii] == bits->getUnary());

  fprintf(stderr, "Tested.\n");

  //while (1)
  //  ;

  delete    bits;
  delete [] random;
}





void
testBinary(uint32 testSize) {
  uint32      maxN   = 10000000;
  uint32     *width  = new uint32 [maxN];
  uint64     *random = new uint64 [maxN];
  mtRandom    mt;

  fprintf(stderr, "Creating.\n");

  uint64  total = 0;

  for (uint32 ii=0; ii<maxN; ii++) {
    width[ii]   = mt.mtRandom32() % testSize;
    random[ii]  = mt.mtRandom64() & (((uint64)1 << width[ii]) - 1);
    total      +=  width[ii];
  }

  fprintf(stderr, "Created " F_U64 " Mbits, " F_U64 " MB needed.\n", total >> 20, (total + maxN * sizeof(uint32) * 8) >> 23);

  total = 0;

  fprintf(stderr, "Setting.\n");

  stuffedBits *bits = new stuffedBits;

  for (uint32 ii=0; ii<maxN; ii++) {
    bits->setBinary(width[ii], random[ii]);
    total += width[ii];

    assert(bits->getPosition() == total);
  }

  fprintf(stderr, "Testing.\n");

  bits->setPosition(0);

  for (uint32 ii=0; ii<maxN; ii++) {
    uint64  b = bits->getBinary(width[ii]);
    assert(random[ii] == b);
  }

  fprintf(stderr, "Tested.\n");

  //while (1)
  //  ;

  delete    bits;
  delete [] random;
  delete [] width;
}









void
testPrefixFree(uint32 type) {
  uint32      maxN   = 100000000;
  uint64      length = 0;
  uint32     *width  = new uint32 [maxN];
  uint64     *random = new uint64 [maxN];
  uint64     *histo  = new uint64 [65];
  mtRandom    mt;

  fprintf(stderr, "Creating.\n");

  //  need to limit the number of bits in each value,
  //  but not as aggressively as above.

  for (uint32 ii=0; ii<65; ii++)
    histo[ii] = 0;

  for (uint32 ii=0; ii<maxN; ii++) {
    width[ii]   =  mt.mtRandom32() % 64 + 1;

#if 0
    double x = mt.mtRandomGaussian(0.0, 1.0) * 27 / 6;

    if (x < 0)
      x = -x;

    width[ii]   = (uint32)ceil(x);

    if (width[ii] > 27)
      width[ii] = 27;

    if (width[ii] > 64)
      width[ii] = 64;
#endif

    length     += width[ii];
    histo[width[ii]]++;

    random[ii]  =  mt.mtRandom64() & buildLowBitMask<uint64>(width[ii]);

    if (random[ii] == 0)
      ii--;
  }

  {
    FILE *F = fopen("length.histo", "w");
    for (uint32 ii=0; ii<65; ii++)
      fprintf(F, "%u\t%lu\n", ii, histo[ii]);
    fclose(F);
  }

  if (0) {
    FILE *F = fopen("length.dat", "w");
    for (uint32 ii=0; ii<maxN; ii++)
      fprintf(F, "%u\n", width[ii]);
    fclose(F);
  }

  fprintf(stderr, "Created %u numbers with total length %lu bits.\n", maxN, length);

  fprintf(stderr, "Setting.\n");

  stuffedBits *bits = new stuffedBits;

  for (uint32 ii=0; ii<maxN; ii++) {
    switch (type) {
      case 0:
        bits->setEliasGamma(random[ii]);
        break;
      case 1:
        bits->setEliasDelta(random[ii]);
        break;
      case 2:
        bits->setZeckendorf(random[ii]);
        break;
    }

    //assert(bits->getPosition() == total);
  }

  fprintf(stderr, "Set %lu bits.\n", bits->getPosition());
  fprintf(stderr, "Testing.\n");

  bits->setPosition(0);

  for (uint32 ii=0; ii<10; ii++)
    fprintf(stderr, "value %2u %22lu width %2u\n", ii, random[ii], width[ii]);

  for (uint32 ii=0; ii<5; ii++)
    fprintf(stderr, "word %2u %s\n", ii, bits->displayWord(ii));

  for (uint32 ii=0; ii<maxN; ii++) {
    uint64  b = 0;

    switch (type) {
      case 0:
        b = bits->getEliasGamma();
        break;
      case 1:
        b = bits->getEliasDelta();
        break;
      case 2:
        b = bits->getZeckendorf();
        break;
    }

    if (b != random[ii])
      fprintf(stderr, "Failed at ii %u expect random=%lu got b=%lu\n",
              ii, random[ii], b);
    assert(random[ii] == b);
  }

  fprintf(stderr, "Tested.\n");

  delete    bits;
  delete [] random;
  delete [] width;
}







//  Just a useful report of the fibonacci numbers.
void
showFibonacciNumbers(void) {
  uint64  _fibDataMax = 93;
  uint64 *_fibData    = new uint64 [_fibDataMax + 1];

  _fibData[0] = 1;
  _fibData[1] = 1;

  for (uint32 ii=2; ii<_fibDataMax; ii++) {
    _fibData[ii] = _fibData[ii-1] + _fibData[ii-2];

    fprintf(stderr, "%4u -- %22lu = %22lu + %22lu -- %22lu\n",
            ii, _fibData[ii], _fibData[ii-1], _fibData[ii-2], UINT64_MAX - _fibData[ii]);

    assert(_fibData[ii] > _fibData[ii-1]);
  }
}





void
testExpandCompress(void) {
  uint64   o = 0x0000000000000003llu;
  uint64   e = 0;
  uint64   c = 0;

  for (uint32 xx=0; xx<22; xx++) {
    if (xx == 21)
      fprintf(stdout, "A successful test will now fail an assert in function expandTo3().\n");

    e = expandTo3(o);
    c = compressTo2(e);

    if (c != o)
      fprintf(stdout, "orig 0x%s expanded %022lo compressed 0x%s FAIL\n", toHex(o), e, toHex(c));
    assert(c == o);

    fprintf(stdout, "orig 0x%s expanded %021lo compressed 0x%s\n", toHex(o), e, toHex(c));

    o <<= 2;
  }
}













int
main(int argc, char **argv) {
  int32 arg=1;
  int32 err=0;

  omp_set_num_threads(1);

  while (arg < argc) {
    if      (strcmp(argv[arg], "-h") == 0) {
      err++;
    }

    else if (strcmp(argv[arg], "-masks") == 0) {
      testMasks();
    }

    else if (strcmp(argv[arg], "-logbasetwo") == 0) {
      testLogBaseTwo();
    }

    else if (strcmp(argv[arg], "-clear") == 0) {
      testSaveClear();
    }

    else if (strcmp(argv[arg], "-bitarray") == 0) {
      if (++arg >= argc)
        fprintf(stderr, "ERROR: -bitarray needs word-size argument.\n"), exit(1);

      testBitArray(strtouint64(argv[arg]));
    }

    else if (strcmp(argv[arg], "-wordarray") == 0) {
      if (++arg >= argc)
        fprintf(stderr, "ERROR: -wordarray needs word-size argument.\n"), exit(1);

      testWordArray(strtouint64(argv[arg]));
    }

    else if (strcmp(argv[arg], "-unary") == 0) {
      if (++arg >= argc)
        fprintf(stderr, "ERROR: -unary needs max-size argument.\n"), exit(1);

      uint32  maxSize = strtouint32(argv[arg]);

#pragma omp parallel for
      for (uint32 xx=1; xx<=maxSize; xx++) {
        fprintf(stderr, "TESTING %u out of %u.\n", xx, maxSize);
        testUnary(xx);
      }
    }

    else if (strcmp(argv[arg], "-binary") == 0) {
#pragma omp parallel for
      for (uint32 xx=1; xx<=64; xx++) {
        fprintf(stderr, "TESTING %u out of %u.\n", xx, 64);
        testBinary(xx);
      }
    }

    else if (strcmp(argv[arg], "-eliasgamma") == 0) {
      testPrefixFree(0);
    }

    else if (strcmp(argv[arg], "-eliasdelta") == 0) {
      testPrefixFree(1);
    }

    else if (strcmp(argv[arg], "-zeckendorf") == 0) {
      testPrefixFree(2);
    }

    else if (strcmp(argv[arg], "-show-fibonacci") == 0) {
      showFibonacciNumbers();
    }

    else if (strcmp(argv[arg], "-expand") == 0) {
      testExpandCompress();
    }

    else if (strcmp(argv[arg], "") == 0) {
    }

    else {
      err++;
    }

    arg++;
  }

  if (err)
    fprintf(stderr, "ERROR: didn't parse command line.\n"), exit(1);

  exit(0);
}
