
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

#include "dnaSeq-v1.H"

namespace merylutil::inline sequence::inline v1 {

dnaSeq::dnaSeq() {
};


dnaSeq::~dnaSeq() {
  delete [] _name;
  delete [] _seq;
  delete [] _qlt;
};


void
dnaSeq::releaseAll(void) {
  delete [] _name;    _name = _ident = _flags = nullptr;
  delete [] _seq;     _seq                    = nullptr;
  delete [] _qlt;     _qlt                    = nullptr;

  _nameMax = 0;
  _seqMax  = 0;
  _seqLen  = 0;
}


void
dnaSeq::releaseBases(void) {
  delete [] _seq;     _seq                    = nullptr;
  delete [] _qlt;     _qlt                    = nullptr;

  _seqMax  = 0;
  _seqLen  = 0;
}


bool
dnaSeq::copy(char  *bout,
             uint32 bgn, uint32 end, bool terminate) {

  if ((end < bgn) || (_seqLen < end))
    return(false);

  for (uint32 ii=bgn; ii<end; ii++)
    bout[ii-bgn] = _seq[ii];

  if (terminate)
    bout[end-bgn] = 0;

  return(true);
}


bool
dnaSeq::copy(char  *bout,
             uint8 *qout,
             uint32 bgn, uint32 end, bool terminate) {

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
}


void
dnaSeq::findNameAndFlags(void) {
  uint32 ii=0;

  while (isWhiteSpace(_name[ii]) == true)   //  Skip white space before the name.
    ii++;                                   //  Why do you torture us?

  _ident = _name + ii;                      //  At the start of the name.

  while (isVisible(_name[ii]) == true)      //  Skip over the name.
    ii++;

  if (isNUL(_name[ii]) == true) {           //  If at the end of the string,
    _flags = _name + ii;                    //  there are no flags,
    return;                                 //  so just return.
  }

  _name[ii++] = 0;                          //  Terminate the name, move ahead.

  while (isWhiteSpace(_name[ii]) == true)   //  Otherwise, skip whitespace
    ii++;                                   //  to get to the flags.

  _flags = _name + ii;                      //  Flags are here or NUL.
}

}  //  namespace merylutil::sequence::v1
