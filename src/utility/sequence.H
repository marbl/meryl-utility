
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



struct dnaSeqIndexEntry;   //  Internal use only, sorry.

class dnaSeq {
public:
  dnaSeq() {
    _nameMax = 0;
    _name    = NULL;
    _seqMax  = 0;
    _seq     = NULL;
    _qlt     = NULL;
    _seqLen  = 0;
  };

  ~dnaSeq() {
    delete [] _name;
    delete [] _seq;
    delete [] _qlt;
  };


  char             *name(void)         { return(_name);   };
  char             *bases(void)        { return(_seq);    };
  uint8            *quals(void)        { return(_qlt);    };

  uint64            length(void)       { return(_seqLen); };

  //  Copy bases from bgn to end, but not including end.
  //  Will NUL-terminate only if requested.
  //  Returns false if bgn or end are out of range or inconsistent.
  //
  bool              copy(char  *bout,
                         uint32 bgn, uint32 end,
                         bool terminate = true) {
    if ((end < bgn) || (_seqLen < end))
      return(false);

    for (uint32 ii=bgn; ii<end; ii++)
      bout[ii-bgn] = _seq[ii];

    if (terminate)
      bout[end-bgn] = 0;

    return(true);
  };

  bool              copy(char  *bout,
                         uint8 *qout,
                         uint32 bgn, uint32 end,
                         bool terminate = true) {
    if ((end < bgn) || (_seqLen < end))
      return(false);

    for (uint32 ii=bgn; ii<end; ii++) {
      bout[ii-bgn] = _seq[ii];
      qout[ii-bgn] = _qlt[ii];
    }

    if (terminate) {
      bout[end-bgn] = 0;
      qout[end-bgn] = 0;
    }

    return(true);
  };


private:
  uint32            _nameMax;
  char             *_name;
  uint64            _seqMax;
  char             *_seq;
  uint8            *_qlt;
  uint64            _seqLen;

  friend class dnaSeqFile;
};




class dnaSeqFile {
public:
  dnaSeqFile(char const *filename, bool indexed=false);
  ~dnaSeqFile();

  void     reopen(void);

  void     generateIndex(void);

  //  If indexed, searches the index for the proper sequence.
  //
  //  If not indexed, searches forward in the file for the sequence.  If not found,
  //  the file will be at the end.
  //
  //  In both cases, the file is left positioned at the start of the sequence header.
  //
  //  Returns true if found, false if not.
  //
  bool        findSequence(uint64 i);
  bool        findSequence(char const *name);

  //  Returns the number of sequences in the file.
  uint64      numberOfSequences(void) {
    return(_indexLen);
  };

  //  Returns the length of sequence i.  If no such sequence, returns UINT64_MAX.
  uint64      sequenceLength(uint64 i);

  char const *filename(void) {
    return(_filename);
  }

public:
  //  Return true if the last sequence loaded looks like FASTA or FASTQ.
  //  If no sequence has been loaded yet, BOTH functions will return false.
  //
  bool   isFASTA(void)   { return(_isFASTA); };
  bool   isFASTQ(void)   { return(_isFASTQ); };

  //  Return the next sequence in the file.
  //  Returns false if EOF, true otherwise, even if the sequence is length zero.
  //
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

  //  Returns a chunk of sequence from the file, up to 'maxLength' bases or
  //  the end of the current sequence.  This is NOT NUL terminated!
  //
  //  Returns false if EOF is hit and no bases were loaded.
  //
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

  dnaSeqIndexEntry      *_index    = nullptr;
  uint64                 _indexLen = 0;
  uint64                 _indexMax = 0;

};


#endif  //  LIBSEQUENCE_H
