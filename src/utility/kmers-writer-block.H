
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

//  Responsible for taking a block of sorted kmers
//  and writing them to the appropriate file.

class kmerCountFileWriter;

class kmerCountBlockWriter {
public:
  kmerCountBlockWriter(kmerCountFileWriter *writer);
  ~kmerCountBlockWriter();

public:
  void    addBlock(uint64  prefix, uint64  nKmers, uint64 *suffixes, uint32 *values);
  void    addBlock(uint64  prefix, uint64  nKmers, uint64 *suffixes, uint64 *values);

  void    finishBatch(void);
  void    finish(void);

private:
  void    closeFileDumpIndex(uint32 oi, uint32 iteration=UINT32_MAX);
  void    mergeBatches(uint32 oi);

private:
  kmerCountFileWriter       *_writer;
  char                       _outName[FILENAME_MAX+1];

  //  Encoding data

  uint32                     _prefixSize;

  uint32                     _suffixSize;
  uint64                     _suffixMask;

  uint32                     _numFilesBits;
  uint32                     _numBlocksBits;
  uint64                     _numFiles;
  uint64                     _numBlocks;

  //  File data

  FILE                     **_datFiles;
  kmerCountFileIndex       **_datFileIndex;

  //  Kmer data and etc

  uint32                     _iteration;
};



