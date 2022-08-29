
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

class md5sum {
public:
  md5sum();
  md5sum(uint8 const *b, uint32 bLen);
  ~md5sum();

  char const *toString(void);
  void        toString(char *str);

  //void        addByte(uint8 b);
  void        addBlock(uint8 *b, uint64 bLen);

  void        finalize(void);

private:
  uint8       digest[33] = {0};
  char        dascii[33] = {0};

  uint8      *buffer    = nullptr;
  uint8       bufferLen = 0;
  uint8       bufferMax = 0;

  void       *context;
};
