
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

#ifndef MERYLUTIL_SEQUENCE_V1_DNASEQFILE_H
#define MERYLUTIL_SEQUENCE_V1_DNASEQFILE_H

#include "types.H"
#include "files.H"

#include "dnaSeq-v1.H"

//
//  An interface to FASTA and FASTQ files.
//
//  Upon object creation, you can request that an index of the file be
//  generated.  Without an index, numberOfSequences(), findSequence() and
//  sequenceLength() do not work well or at all.
//
//  generateIndex() will force an index to be generated.
//  removeIndex will remove any index.
//
//  reopen() will reset the file to the start and.  If the 'indexed' flag is
//  true, or an index already exists, an index is (re)created.  Note that
//  setting 'indexed=false' will NOT remove an existing index.
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

namespace merylutil::inline sequence::inline v1 {

class dnaSeqFile {
public:
  dnaSeqFile(char const *filename, bool indexed=false);
  ~dnaSeqFile();

  void        reopen(bool indexed=false);
  void        generateIndex(void);
  void        removeIndex(void);

public:
  char const *filename(void)            { return(_filename); };
  uint64      numberOfSequences(void)   { return(_indexLen); };

  bool        findSequence(uint64 i);
  uint64      sequenceLength(uint64 i);

public:
  //  True if the last sequence loaded was from a FASTA or FASTQ file.
  bool   isFASTA(void)      { return(_isFASTA); };
  bool   isFASTQ(void)      { return(_isFASTQ); };

  //  Return the sequence index of the last loaded sequence.
  uint64 seqIdx(void)       { return(_seqIdx-1); };

  //  True if the input file is compressed (gzip, xz, etc).
  bool   isCompressed(void) { return(_file->isCompressed() == true); };
  bool   isIndexable(void)  { return(_file->isNormal()     == true); };

public:
  bool   loadSequence(char   *&name, uint32 &nameMax,
                      char   *&seq,
                      uint8  *&qlt,  uint64 &seqMax, uint64 &seqLen, uint32 &errorCode);
  bool   loadSequence(dnaSeq &seq);

public:
  bool   loadBases(char    *seq,
                   uint64   maxLength,
                   uint64  &seqLength,
                   bool    &endOfSequence);

private:
  bool     loadIndex(void);
  void     saveIndex(void);

  bool
  loadFASTA(char  *&name, uint32 &nameMax,
            char  *&seq,
            uint8 *&qlt,  uint64 &seqMax, uint64 &seqLen, uint64 &qltLen);

  bool
  loadFASTQ(char  *&name, uint32 &nameMax,
            char  *&seq,
            uint8 *&qlt,  uint64 &seqMax, uint64 &seqLen, uint64 &qltLen);

private:
  char                  *_filename = nullptr;

  bool                   _isFASTA  = false;
  bool                   _isFASTQ  = false;
  uint64                 _seqIdx   = 0;

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

}  //  namespace merylutil::sequence::v1

#endif  //  MERYLUTIL_SEQUENCE_V1_DNASEQFILE_H
