
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

#ifndef LIBSEQUENCE_H
#define LIBSEQUENCE_H

#include "types.H"
#include "files.H"


void   reverseComplementSequence(char *seq, int len);
char  *reverseComplementCopy(char *seq, int len);

//  Used in tgTig still.
template<typename qvType>
void   reverseComplement(char *seq, qvType *qlt, int len);

uint32 homopolyCompress(char *bases, uint32 basesLen, char *compr=NULL, uint32 *ntoc=NULL, char skip=0x7f);


//  Encode a sequence into chunk.  The length of the chunk in bytes is returned.
//  If the chunk is NULL, it is allocated.  Otherwise, it must be
//  at least seqLen bytes in length.
uint32 encode2bitSequence(uint8 *&chunk, char *seq, uint32 seqLen);
uint32 encode3bitSequence(uint8 *&chunk, char *seq, uint32 seqLen);
uint32 encode8bitSequence(uint8 *&chunk, char *seq, uint32 seqLen);


//  Decode an encoded sequence (in chunk) of length chunkLen.
//  seq must be allocated to have seqLen+1 bytes.
//  seqLen must be the length of the sequence to decode.
void   decode2bitSequence(uint8 *chunk, uint32 chunkLen, char *seq, uint32 seqLen);
void   decode3bitSequence(uint8 *chunk, uint32 chunkLen, char *seq, uint32 seqLen);
void   decode8bitSequence(uint8 *chunk, uint32 chunkLen, char *seq, uint32 seqLen);


//  Encode/decode an ACGT base to 0132.  Relies on the ASCII encoding:
//
//    A      a     01c0 000 1 == 0 -> 0
//    C      c     01c0 001 1 == 1 -> 1
//    T      t     01c1 010 0 == 2 -> 2
//    G      g     01c0 011 1 == 3 -> 3
//    N      n     01c0 111 0 == 7 -> 4
//                      ^^^
//  Decoding will always return uppercase letters (c=0).
//
//  The inline arrays, in gcc anyway, compile to a single 64-bit constant
//  and is equivalent to the C code:
//
//    0x0706050403020100llu >> (((base >> 1) & 0x07) << 3) & 0x0f
//
//  with the additional optimization of removing the redundant shifts.

inline
uint8
encode2bitBase(char  base) {
  return((uint8 [8]){0, 1, 2, 3, 4, 4, 4, 4}[base >> 1 & 0x07]);
}

inline
char
decode2bitBase(uint8 base) {
  return("ACTGNNNN"[base & 0x07]);
}



//  A sequence loaded from disk.
//
//  If quality values are not available (e.g., FASTA) then
//  all values are set to zero.
//
//  The copy functions will copy bases (and qualities) from bgn to end, but
//  not including the base at end -- that is, normal C-style semantics.
//  The output will be NUL-terminated, unless explicitly told not to.
//  Returns false if bgn or end are out of range or inconsistent.
//
class dnaSeq {
public:
  dnaSeq();
  ~dnaSeq();

  char             *name(void)         { return(_name);   };
  char             *bases(void)        { return(_seq);    };
  uint8            *quals(void)        { return(_qlt);    };

  uint64            length(void)       { return(_seqLen); };

  bool              copy(char  *bout,
                         uint32 bgn, uint32 end, bool terminate = true);

  bool              copy(char  *bout,
                         uint8 *qout,
                         uint32 bgn, uint32 end, bool terminate = true);

private:
  char             *_name    = nullptr;
  uint32            _nameMax = 0;

  char             *_seq     = nullptr;
  uint8            *_qlt     = nullptr;
  uint64            _seqMax  = 0;         //  Space allocated.
  uint64            _seqLen  = 0;         //  Actual length.

  friend class dnaSeqFile;
};



//  An interface to FASTA and FASTQ files.
//
//  findSequence() will return true if the specified sequence is found in the
//  file and leave the file positioned such that the next loadSequence() will
//  load that sequence.
//   - If an index exists, the index will be searched and the sequence will
//     be returned regardless of where it is in the file.
//   - If no index exists, the file will be searched forward until the
//     sequence is found or the file ends.  It is not possible to move
//     'backward' in the file in this case.
//
//  sequenceLength() will return the length of sequence index i.  If no index
//  exists, or i is not a valid sequence index, UINT64_MAX is returned.
//
//  isFASTA() and isFASTQ() return true if the last sequence loaded came from
//  a FASTA or FASTQ source, respectively.  If no sequence has been loaded
//  yet, both functions will return false.
//
//  loadSequence() will read the next sequence from the file.  Returns false
//  if the end of file is encountered, true otherwise.  In particular, a
//  sequence of length zero will return true.
//
//  loadBases() will return a chunk of sequence from the file, up to
//  'maxLength' bases or the end of the current sequence.
//   - Returns false only if EOF is encountered.
//   - seqLength will have the length of the sequence returned.  This can be zero.
//   - endOfSequence will be true if the end of the sequence was encountered.
//   - The returned sequence is NOT NUL terminated.
//

class dnaSeqFile {
public:
  dnaSeqFile(char const *filename, bool indexed=false);
  ~dnaSeqFile();

  void        reopen(void);
  void        generateIndex(void);

public:
  char const *filename(void)            { return(_filename); };
  uint64      numberOfSequences(void)   { return(_indexLen); };

  bool        findSequence(uint64 i);
  uint64      sequenceLength(uint64 i);

public:
  bool   isFASTA(void)   { return(_isFASTA); };
  bool   isFASTQ(void)   { return(_isFASTQ); };

public:
  bool   loadSequence(char   *&name,     uint32  &nameMax,
                      char   *&seq,
                      uint8  *&qlt,      uint64  &seqMax,
                      uint64  &seqLen);

  bool   loadSequence(dnaSeq &seq) {
    return(loadSequence(seq._name,    seq._nameMax,
                        seq._seq,
                        seq._qlt,     seq._seqMax,
                        seq._seqLen));
  };

public:
  bool   loadBases(char    *seq,
                   uint64   maxLength,
                   uint64  &seqLength,
                   bool    &endOfSequence);

private:
  bool     loadIndex(void);
  void     saveIndex(void);

  uint64
  loadFASTA(char   *&name,     uint32  &nameMax,
            char   *&seq,
            uint8  *&qlt,      uint64  &seqMax);

  uint64
  loadFASTQ(char   *&name,     uint32  &nameMax,
            char   *&seq,
            uint8  *&qlt,      uint64  &seqMax);

private:
  char                  *_filename = nullptr;
  bool                   _indexed  = false;

  bool                   _isFASTA  = false;
  bool                   _isFASTQ  = false;

  compressedFileReader  *_file     = nullptr;
  readBuffer            *_buffer   = nullptr;

  struct dnaSeqIndexEntry {     //  Offset of the first byte in the record:
    uint64   _fileOffset;       //  '>' for FASTA, '@' for fastq.
    uint64   _sequenceLength;   //
  };

  dnaSeqIndexEntry      *_index    = nullptr;
  uint64                 _indexLen = 0;
  uint64                 _indexMax = 0;

};


#endif  //  LIBSEQUENCE_H
