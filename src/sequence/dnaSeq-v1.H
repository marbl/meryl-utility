
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

#ifndef MERYLUTIL_SEQUENCE_V1_DNASEQ_H
#define MERYLUTIL_SEQUENCE_V1_DNASEQ_H

#include "types.H"

//
//  A sequence loaded from disk.  It should be treated as a read-only object.
//
//  ident() returns the first word of the sequence header line, while flags()
//  returns the rest of the line, or an empty line if there is no more line.
//
//  It isn't possible to modify ident() and flags().  They're pointers
//  into the same memory, and that isn't exposed.
//
//  bases() and quals() could support modifications, as long as the length of
//  the string doesn't change.  Only canu needed to do that and it was worked
//  around.
//
//  If quality values are not available (e.g., FASTA) then all values are set
//  to zero.
//
//  The copy functions will copy bases (and qualities) from bgn to end, but
//  not including the base at end -- that is, normal C-style semantics.  The
//  output will be NUL-terminated, unless explicitly told not to.  Returns
//  false if bgn or end are out of range or inconsistent.
//

namespace merylutil::sequence::inline v1 {
class dnaSeq {
public:
  dnaSeq();
  ~dnaSeq();

  char  const      *ident(void)        { return(_ident);  };
  char  const      *flags(void)        { return(_flags);  };
  char  const      *bases(void)        { return(_seq);    };
  uint8 const      *quals(void)        { return(_qlt);    };

  uint64            length(void)       { return(_seqLen); };

  void              releaseAll(void);      //  Release all memory.
  void              releaseBases(void);    //  Release seq memory; keep the name.

  bool              copy(char  *bout,
                         uint32 bgn, uint32 end, bool terminate = true);

  bool              copy(char  *bout,
                         uint8 *qout,
                         uint32 bgn, uint32 end, bool terminate = true);

  bool              wasError(void)     { return((_error & 0x01) == 0x01); };
  bool              wasReSync(void)    { return((_error & 0x02) == 0x02); };

private:
  void              findNameAndFlags(void);

private:
  char             *_name    = nullptr;
  uint32            _nameMax = 0;

  char             *_ident   = nullptr;
  char             *_flags   = nullptr;

  char             *_seq     = nullptr;
  uint8            *_qlt     = nullptr;
  uint64            _seqMax  = 0;         //  Space allocated.
  uint64            _seqLen  = 0;         //  Actual length.

  uint32            _error   = 0;

  friend class dnaSeqFile;
};
}


#endif  //  MERYLUTIL_SEQUENCE_V1_DNASEQ_H
