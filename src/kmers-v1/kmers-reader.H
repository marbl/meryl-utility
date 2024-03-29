
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

#ifndef MERYLUTIL_KMERS_READER_V1_H
#define MERYLUTIL_KMERS_READER_V1_H

#ifndef MERYLUTIL_KMERS_H
#error "include kmers.H, not this."
#endif

namespace merylutil::inline kmers::v1 {

class merylFileReader {
private:
  void    initializeFromMasterI_v00(void);
  void    initializeFromMasterI_v01(stuffedBits  *masterIndex, bool doInitialize);
  void    initializeFromMasterI_v02(stuffedBits  *masterIndex, bool doInitialize);
  void    initializeFromMasterI_v03(stuffedBits  *masterIndex, bool doInitialize);
  void    initializeFromMasterIndex(bool  doInitialize, bool  loadStatistics, bool  beVerbose);

public:
  merylFileReader(const char *inputName,
                      bool        beVerbose=false);

  merylFileReader(const char *inputName,
                      uint32      threadFile,
                      bool        beVerbose=false);

  ~merylFileReader();

public:
  void    rewind(void) {
    _activeMer  = 0;    //  Position we are at in the block loaded.
    _activeFile = 0;

    _nKmers     = 0;    //  Kmers in the block we have loaded.

    if (_threadFile != UINT32_MAX)
      _activeFile = _threadFile;

    closeFile(_datFile);
  };

public:
  void    loadStatistics(void);
  void    dropStatistics(void);

public:
  void    enableThreads(uint32 threadFile);

public:
  void    loadBlockIndex(void);

public:
  bool    nextMer(void);
  kmer    theFMer(void)        { return(_kmer);        };
  kmvalu  theValue(void)       { return(_value);       };

  bool    isMultiSet(void)     { return(_isMultiSet);  };

  char   *filename(void)       { return(_inName);      };

  merylHistogram  *stats(void) {
    loadStatistics();
    return(_stats);
  }

  //  For direct access to the kmer blocks.
public:
  uint32  prefixSize(void)     { return(_prefixSize); };
  uint32  suffixSize(void)     { return(_suffixSize); };

  uint32  numFiles(void)       { return(_numFiles);  };
  uint32  numBlocks(void)      { return(_numBlocks); };

  uint32  numFilesBits(void)   { return(_numFilesBits);  };
  uint32  numBlocksBits(void)  { return(_numBlocksBits); };

  FILE   *blockFile(uint32 ff) {
    FILE *F = NULL;

    if (ff < _numFiles)
      F = openInputBlock(_inName, ff, _numFiles);

    return(F);
  };

  merylFileIndex   &blockIndex(uint32 bb) {
    return(_blockIndex[bb]);
  };

private:
  char                       _inName[FILENAME_MAX+1];

  uint32                     _prefixSize;
  uint32                     _suffixSize;
  uint32                     _numFilesBits;
  uint32                     _numBlocksBits;

  uint32                     _numFiles;
  uint32                     _numBlocks;

  bool                       _isMultiSet;

  merylHistogram            *_stats;

  FILE                      *_datFile;

  merylFileBlockReader      *_block;
  merylFileIndex            *_blockIndex;

  kmer                       _kmer;
  kmvalu                     _value;

  uint64                     _prefix;

  kmdata                     _activeMer;
  uint32                     _activeFile;

  uint32                     _threadFile;

  uint64                     _nKmers;
  uint64                     _nKmersMax;
  kmdata                    *_suffixes;
  kmvalu                    *_values;
};

}  //  namespace merylutil::kmers::v1

#endif  //  MERYLUTIL_KMERS_READER_V1_H
