
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

#ifndef MERYLUTIL_SEQUENCE_V1_H
#define MERYLUTIL_SEQUENCE_V1_H

#include "types.H"
#include "files.H"

namespace merylutil::inline sequence::inline v1 {

//////////
//
//  Various functions for reverse complementing sequences.
//    reverseComplementSequence() is in place.
//    reverseComplementCopy()     returns a 'new' character array of length 'len+1'.
//    reverseComplement()         reverses both sequence and bases; used in tgTig still.
//
void   reverseComplementSequence(char *seq, int len);
char  *reverseComplementCopy    (char *seq, int len);

template<typename qvType>
void   reverseComplement(char *seq, qvType *qlt, int len);


//////////
//
//  Compress homopolymer runs to a single letter.  More comments in the .C file.
//
uint32 homopolyCompress(char   *bases, uint32 basesLen,
                        char   *compr=nullptr,
                        uint32 *ntoc=nullptr, char skip=0x7f);

//////////
//
//  Encode/decode sequence between ASCII and binary.
//    Encoding accepts upper or lower case letters.
//    Decoding ALWAYS returns uppercase letters (c=0).
//
//  The two and three bit encodings maps ACGTN to 01324 and relies on the
//  ASCII encoding (where 'c' denotes uppercase (c=0) or lowercase (c=1)):
//      A      a     01c0 000 1 == 0 -> 0
//      C      c     01c0 001 1 == 1 -> 1
//      T      t     01c1 010 0 == 2 -> 2
//      G      g     01c0 011 1 == 3 -> 3
//      N      n     01c0 111 0 == 7 -> 4
//                        ^^^
//
//  The inline arrays, in gcc anyway, compile to a single 64-bit constant
//  and is equivalent to the C code (with the redundant shifts optimized out):
//    0x0706050403020100llu >> (((base >> 1) & 0x07) << 3) & 0x0f
//
inline uint8 encode2bitBase(char  base) { return((uint8 [8]){0, 1, 2, 3, 4, 4, 4, 4}[base >> 1 & 0x07]); }
inline char  decode2bitBase(uint8 base) { return("ACTGNNNN"[base & 0x07]); }

uint32 encode2bitSequence(uint8 *&chunk, char const *seq, uint32 seqLen);
uint32 encode3bitSequence(uint8 *&chunk, char const *seq, uint32 seqLen);
uint32 encode8bitSequence(uint8 *&chunk, char const *seq, uint32 seqLen);

void   decode2bitSequence(uint8 const *chunk, uint32 chunkLen, char *seq, uint32 seqLen);
void   decode3bitSequence(uint8 const *chunk, uint32 chunkLen, char *seq, uint32 seqLen);
void   decode8bitSequence(uint8 const *chunk, uint32 chunkLen, char *seq, uint32 seqLen);

}  //  merylutil::sequence::v1

#endif  //  MERYLUTIL_SEQUENCE_V1_H
