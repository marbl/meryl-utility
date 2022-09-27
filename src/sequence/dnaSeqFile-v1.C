
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

#include "dnaSeqFile-v1.H"

#include "arrays.H"
#include "strings.H"


merylutil::sequence::v1::dnaSeqFile::dnaSeqFile(char const *filename, bool indexed) {
  _filename = duplicateString(filename);

  reopen(indexed);
}



merylutil::sequence::v1::dnaSeqFile::~dnaSeqFile() {
  delete [] _filename;
  delete    _file;
  delete    _buffer;
  delete [] _index;
}



//  Open, or reopen, an input file.
//
void
merylutil::sequence::v1::dnaSeqFile::reopen(bool indexed) {

  //  If a _file exists already, reopen it, otherwise, make a new one.
  if (_file)
    _file->reopen();
  else
    _file = new compressedFileReader(_filename);

  //  Since the file object is always new, we need to make a new read buffer.
  //  gzip inputs seem to be (on FreeBSD) returning only 64k blocks
  //  regardless of the size of our buffer; but uncompressed inputs will
  //  benefit slightly from a bit larger buffer.
  delete _buffer;

  _buffer = new readBuffer(_file->file(), 128 * 1024);

  //  If we have an index already or one is requested, (re)generate it.

  if ((_index != nullptr) || (indexed == true))
    generateIndex();
}



bool
merylutil::sequence::v1::dnaSeqFile::findSequence(uint64 i) {

  if (_indexLen == 0)   return(false);
  if (_indexLen <= i)   return(false);

  _buffer->seek(_index[i]._fileOffset);

  _seqIdx = i;

  return(true);
}



uint64
merylutil::sequence::v1::dnaSeqFile::sequenceLength(uint64 i) {

  if (_indexLen == 0)   return(UINT64_MAX);
  if (_indexLen <= i)   return(UINT64_MAX);

  return(_index[i]._sequenceLength);
}




////////////////////////////////////////
//  dnaSeqFile indexing
//

const uint64 dnaSeqVersion01 = 0x3130716553616e64;   //  dnaSeq01
const uint64 dnaSeqVersion02 = 0x3230716553616e64;   //  dnaSeq02 - not used yet


char const *
makeIndexName(char const *prefix) {
  char const *suffix = ".dnaSeqIndex";
  uint32      plen   = strlen(prefix);
  uint32      slen   = strlen(suffix);
  char       *iname  = new char [plen + slen + 1];

  memcpy(iname,        prefix, plen + 1);   //  +1 for the NUL byte.
  memcpy(iname + plen, suffix, slen + 1);

  return(iname);
}


//  Load an index.  Returns true if one was loaded.
bool
merylutil::sequence::v1::dnaSeqFile::loadIndex(void) {
  char const  *indexName = makeIndexName(_filename);
  FILE        *indexFile = nullptr;

  if (fileExists(indexName) == true) {
    FILE   *indexFile = merylutil::openInputFile(indexName);
    uint64  magic;
    uint64  size;
    uint64  date;

    loadFromFile(magic,     "dnaSeqFile::magic",    indexFile);
    loadFromFile(size,      "dnaSeqFile::size",     indexFile);
    loadFromFile(date,      "dnaSeqFile::date",     indexFile);
    loadFromFile(_indexLen, "dnaSeqFile::indexLen", indexFile);

    if (magic != dnaSeqVersion01) {
      fprintf(stderr, "ERROR: file '%s' isn't a dnaSeqIndex; manually remove this file.\n", indexName);
      exit(1);
    }

    if ((size == merylutil::sizeOfFile(_filename)) &&
        (date == merylutil::timeOfFile(_filename))) {
      _indexMax = _indexLen;
      _index    = new dnaSeqIndexEntry [_indexMax];

      loadFromFile(_index, "dnaSeqFile::index", _indexLen, indexFile);
    }
    else {
      fprintf(stderr, "WARNING: file '%s' disagrees with index; recreating index.\n", _filename);

      _index    = nullptr;
      _indexLen = 0;
      _indexMax = 0;
    }

    merylutil::closeFile(indexFile, indexName);
  }

  delete [] indexName;

  return(_index != nullptr);   //  Return true if we have an index.
}



void
merylutil::sequence::v1::dnaSeqFile::saveIndex(void) {
  char const *indexName = makeIndexName(_filename);
  FILE       *indexFile = merylutil::openOutputFile(indexName);

  uint64  magic = dnaSeqVersion01;
  uint64  size  = merylutil::sizeOfFile(_filename);
  uint64  date  = merylutil::timeOfFile(_filename);

  writeToFile(magic,     "dnaSeqFile::magic",    indexFile);
  writeToFile(size,      "dnaSeqFile::size",     indexFile);
  writeToFile(date,      "dnaSeqFile::date",     indexFile);
  writeToFile(_indexLen, "dnaSeqFile::indexLen", indexFile);
  writeToFile(_index,    "dnaSeqFile::index",    _indexLen, indexFile);

  merylutil::closeFile(indexFile, indexName);

  delete [] indexName;
}



void
merylutil::sequence::v1::dnaSeqFile::generateIndex(void) {
  dnaSeq     seq;

  //  Fail if an index is requested for a compressed file.

  if (_file->isCompressed() == true)
    fprintf(stderr, "ERROR: cannot index compressed input '%s'.\n", _filename), exit(1);

  if (_file->isNormal() == false)
    fprintf(stderr, "ERROR: cannot index pipe input.\n"), exit(1);

  //  If we can load an index, do it and return.

  if (loadIndex() == true)
    return;

  //  Rewind the buffer to make sure we're at the start of the file.

  _buffer->seek(0);

  //  Allocate space for the index, set the first entry to the current
  //  position of the file.

  _indexLen = 0;
  _indexMax = 1048576;
  _index    = new dnaSeqIndexEntry [_indexMax];

  _index[0]._fileOffset     = _buffer->tell();
  _index[0]._sequenceLength = 0;

  //  While we read sequences:
  //    update the length of the sequence (we've already saved the position)
  //    make space for more sequences
  //    save the position of the next sequence

  while (loadSequence(seq) == true) {
    if (seq.wasError()) {
      fprintf(stderr, "WARNING: error reading sequence at/before '%s'\n", seq.ident());
    }

    if (seq.wasReSync()) {
      fprintf(stderr, "WARNING: lost sync reading before sequence '%s'\n", seq.ident());
    }

    _index[_indexLen]._sequenceLength = seq.length();

    increaseArray(_index, _indexLen, _indexMax, 1048576);

    _indexLen++;

    _index[_indexLen]._fileOffset     = _buffer->tell();
    _index[_indexLen]._sequenceLength = 0;
  }

  //  Save whatever index we made.

  saveIndex();
}



void
merylutil::sequence::v1::dnaSeqFile::removeIndex(void) {

  delete [] _index;

  _indexLen = 0;
  _indexMax = 0;
  _index    = nullptr;
}



bool
merylutil::sequence::v1::dnaSeqFile::loadFASTA(char  *&name, uint32 &nameMax,
                                               char  *&seq,
                                               uint8 *&qlt,  uint64 &seqMax, uint64 &seqLen, uint64 &qltLen) {
  uint64  nameLen = 0;
  char    ch      = _buffer->read();

  //  Skip any whitespace.

  while (isWhiteSpace(ch))
    ch = _buffer->read();

  //  Fail rather ungracefully if we aren't at a sequence start.

  if (ch != '>')
    return(false);

  //  Read the header line into the name string.  We cannot skip whitespace
  //  here, but we do allow DOS to insert a \r before any \n.

  for (ch=_buffer->read(); (ch != '\n') && (ch != 0); ch=_buffer->read()) {
    if (ch == '\r')
      continue;
    if (nameLen+1 >= nameMax)
      resizeArray(name, nameLen, nameMax, 3 * nameMax / 2);
    name[nameLen++] = ch;
  }

  //  Trim back the header line to remove white space at the end.  The
  //  terminating NUL is tacked on at the end.

  while ((nameLen > 0) && (isWhiteSpace(name[nameLen-1])))
    nameLen--;

  name[nameLen] = 0;

  //  Read sequence, skipping whitespace, until we hit a new sequence or eof.

  seqLen = 0;
  qltLen = 0;

  for (ch = _buffer->peek(); ((ch != '>') &&
                              (ch != '@') &&
                              (ch !=  0)); ch = _buffer->peek()) {
    assert(_buffer->eof() == false);

    ch = _buffer->read();

    if (isWhiteSpace(ch))
      continue;

    if (seqLen+1 >= seqMax)
      resizeArrayPair(seq, qlt, seqLen, seqMax, 3 * seqMax / 2);

    seq[seqLen++] = ch;
    qlt[qltLen++] = 0;
  }

  seq[seqLen] = 0;
  qlt[qltLen] = 0;

  assert(nameLen < nameMax);
  assert(seqLen  < seqMax);
  assert(qltLen  < seqMax);

  _seqIdx++;

  return(true);
}



bool
merylutil::sequence::v1::dnaSeqFile::loadFASTQ(char  *&name, uint32 &nameMax,
                                               char  *&seq,
                                               uint8 *&qlt,  uint64 &seqMax, uint64 &seqLen, uint64 &qltLen) {
  uint32  nameLen = 0;
  char    ch      = _buffer->read();

  //  Skip any whitespace.

  while (isWhiteSpace(ch))
    ch = _buffer->read();

  //  Fail rather ungracefully if we aren't at a sequence start.

  if (ch != '@')
    return(false);

  //  Read the header line into the name string.  We cannot skip whitespace
  //  here, but we do allow DOS to insert a \r before any \n.

  for (ch=_buffer->read(); (ch != '\n') && (ch != 0); ch=_buffer->read()) {
    if (ch == '\r')
      continue;
    if (nameLen+1 >= nameMax)
      resizeArray(name, nameLen, nameMax, 3 * nameMax / 2);
    name[nameLen++] = ch;
  }

  //  Trim back the header line to remove white space at the end.

  while ((nameLen > 0) && (isWhiteSpace(name[nameLen-1])))
    nameLen--;

  name[nameLen] = 0;

  //  Skip any whitespace, again.  Once we hit non-whitespace we'll suck in
  //  the whole line.

  while (isWhiteSpace(ch))
    ch = _buffer->read();

  //  Read sequence.  Pesky DOS files end with \r\n, and it suffices
  //  to stop on the \n and ignore all the rest.

  seqLen = 0;
  qltLen = 0;

  for (; (ch != '\n') && (ch != 0); ch=_buffer->read()) {
    if (isWhiteSpace(ch))
      continue;
    if (seqLen+1 >= seqMax)
      resizeArrayPair(seq, qlt, seqLen, seqMax, 3 * seqMax / 2);
    seq[seqLen++] = ch;
  }

  //  Skip any more whitespace, fail if we're not at a quality start, then
  //  suck in the quality line.  And then skip more whitespace.

  while (isWhiteSpace(ch))
    ch = _buffer->read();

  if (ch != '+')
    return(false);

  for (ch=_buffer->read(); (ch != '\n') && (ch != 0); ch=_buffer->read()) {
    ;
  }

  while (isWhiteSpace(ch))
    ch = _buffer->read();

  //  Read qualities and convert to integers.

  for (; (ch != '\n') && (ch != 0); ch=_buffer->read()) {
    if (isWhiteSpace(ch))
      continue;
    if (qltLen+1 >= seqMax)
      resizeArrayPair(seq, qlt, qltLen, seqMax, 3 * seqMax / 2);
    qlt[qltLen++] = ch - '!';
  }

  //  Skip whitespace after the sequence.  This one is a little weird.  It
  //  tests if the _next_ letter is whitespace, and if so, gets it from the
  //  buffer.  After this loop, the _next_ letter in the buffer should be
  //  either a '>' or a '@'.

  while (isWhiteSpace(_buffer->peek()))
    _buffer->read();

  seq[seqLen] = 0;
  qlt[qltLen] = 0;

  assert(nameLen < nameMax);
  assert(seqLen  < seqMax);
  assert(qltLen  < seqMax);

  _seqIdx++;

  return(true);
}



bool
merylutil::sequence::v1::dnaSeqFile::loadSequence(char  *&name, uint32 &nameMax,
                                                  char  *&seq,
                                                  uint8 *&qlt,  uint64 &seqMax, uint64 &seqLen, uint32 &error) {
  uint64 qltLen = 0;

  //  Allocate space for the arrays, if they're currently unallocated.

  if (nameMax == 0)
    resizeArray(name, 0, nameMax, (uint32)1024);

  if (seqMax == 0)
    resizeArrayPair(seq, qlt, 0, seqMax, (uint64)65536);

  //  Clear our return values.

  bool   loadSuccess = false;

  _isFASTA = false;
  _isFASTQ = false;

  name[0] = 0;
  seq[0]  = 0;
  qlt[0]  = 0;
  seqLen  = 0;

  error   = 0;

  //  Skip any whitespace at the start of the file, or before the next FASTQ
  //  sequence (the FASTA reader will automagically skip whitespace at the
  //  end of the sequence).

  while (isWhiteSpace(_buffer->peek()))
    _buffer->read();

  //  If we're not at a sequence start, scan ahead to find the next one.
  //  Not bulletproof; FASTQ qv's can match this.

  if ((_buffer->peek() != '>') &&
      (_buffer->peek() != '@') &&
      (_buffer->peek() !=  0)) {
    //fprintf(stderr, "dnaSeqFile::loadSequence()-- sequence sync lost at position %lu, attempting to find the next sequence.\n", _buffer->tell());
    error |= 0x02;
  }

  bool  lastWhite = isWhiteSpace(_buffer->peek());

  while ((_buffer->peek() != '>') &&
         (_buffer->peek() != '@') &&
         (_buffer->peek() !=  0)) {
    _buffer->read();
  }

  //  Peek at the file to decide what type of sequence we need to read.

  if      (_buffer->peek() == '>') {
    _isFASTA    = true;
    loadSuccess = loadFASTA(name, nameMax, seq, qlt, seqMax, seqLen, qltLen);
  }

  else if (_buffer->peek() == '@') {
    _isFASTQ    = true;
    loadSuccess = loadFASTQ(name, nameMax, seq, qlt, seqMax, seqLen, qltLen);
  }

  else {
    _isFASTA = false;
    _isFASTQ = false;

    return(false);
  }

  //  If we failed to load a sequence, report an error message and zero out
  //  the sequence.  Leave the name as-is so we can at least return a length
  //  zero sequence.  If we failed to load a name, it'll still be set to NUL.

  if (loadSuccess == false) {
    //if (name[0] == 0)
    //  fprintf(stderr, "dnaSeqFile::loadSequence()-- failed to read sequence correctly at position %lu.\n", _buffer->tell());
    //else
    //  fprintf(stderr, "dnaSeqFile::loadSequence()-- failed to read sequence '%s' correctly at position %lu.\n", name, _buffer->tell());

    error |= 0x01;

    seq[0]  = 0;
    qlt[0]  = 0;
    seqLen  = 0;
  }

  return(true);
}



bool
merylutil::sequence::v1::dnaSeqFile::loadSequence(dnaSeq &seq) {
  bool result = loadSequence(seq._name, seq._nameMax,
                             seq._seq,
                             seq._qlt,  seq._seqMax, seq._seqLen, seq._error);

  if (result)
    seq.findNameAndFlags();

  return(result);
}



bool
merylutil::sequence::v1::dnaSeqFile::loadBases(char    *seq,
                                               uint64   maxLength,
                                               uint64  &seqLength,
                                               bool    &endOfSequence) {

  seqLength     = 0;
  endOfSequence = false;

  if (_buffer->eof() == true)
    return(false);

  //  If this is a new file, skip the first name line.

  if (_buffer->tell() == 0) {
    while (_buffer->peek() == '\n')    //  Skip whitespace before the first name line.
      _buffer->read();

    _buffer->skipAhead('\n', true);
  }

  //  Skip whitespace.

  while (_buffer->peek() == '\n')
    _buffer->read();

  //  If now at EOF, that's it.

  if  (_buffer->eof() == true)
    return(false);

  //  Otherwise, we must be in the middle of sequence, so load
  //  until we're not in sequence or out of space.

  while (_buffer->eof() == false) {

    //  If we're at the start of a new sequence, skip over any QV's and
    //  the next name line, set endOfSequence and return.

    if (_buffer->peek() == '>') {
      _buffer->skipAhead('\n', true);      //  Skip the name line.
      endOfSequence = true;
      return(true);
    }

    if (_buffer->peek() == '+') {
      _buffer->skipAhead('\n', true);      //  Skip the + line.
      _buffer->skipAhead('\n', true);      //  Skip the QV line.
      _buffer->skipAhead('\n', true);      //  Skip the @ line for the next sequence.
      endOfSequence = true;
      return(true);
    }

    //  Read some bases.

    seqLength += _buffer->copyUntil('\n', seq + seqLength, maxLength - seqLength);

    if (seqLength == maxLength)
      return(true);

    //  We're at a newline (or end of file), either way, suck in the next letter
    //  (or nothing) and keep going.

    _buffer->read();
  }

  //  We hit EOF.  If there are bases loaded, then we're at the end of
  //  a sequence, and should return that we loaded bases.

  endOfSequence = (seqLength > 0);

  return(endOfSequence);
}
