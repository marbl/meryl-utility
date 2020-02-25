
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

//  Responsible for taking one kmer at a time and writing
//  it to

class kmerCountFileWriter;

class kmerCountStreamWriter {
public:
  kmerCountStreamWriter(kmerCountFileWriter *writer,
                        uint32               fileNumber);

  ~kmerCountStreamWriter();

public:
  void    addMer(kmer k, uint64 c);

private:
  void    dumpBlock(uint64 nextPrefix=UINT64_MAX);

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

  uint32                     _filePrefix;

  FILE                      *_datFile;
  kmerCountFileIndex        *_datFileIndex;

  //  Kmer data and etc for writing the stream.

  uint64                     _batchPrefix;
  uint64                     _batchNumKmers;
  uint64                     _batchMaxKmers;
  uint64                    *_batchSuffixes;
  uint64                    *_batchValues;

  //kmerCountStatistics        _stats;
};



