
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

#ifndef MERYLUTIL_FILES_BUFFERED_V1_READING_H
#define MERYLUTIL_FILES_BUFFERED_V1_READING_H

#include "types.H"

//
//  If bMax is zero, then a 32 KB buffer is used.
//

namespace merylutil::inline files::inline v1 {

class readBuffer {
public:
  readBuffer(const char *pfx, char sep, const char *sfx, uint64 bMax = 32 * 1024) { initialize(pfx, sep, sfx, bMax); }
  readBuffer(const char *filename,                       uint64 bMax = 32 * 1024) { initialize(filename, '.', nullptr, bMax); }
  readBuffer(FILE *F,                                    uint64 bMax = 32 * 1024);
  ~readBuffer();

private:
  void          initialize(const char *pfx, char sep, const char *sfx, uint64 bMax);

public:
  bool          eof(void) { return(_eof); }   //  True if next read will hit EOF.

  char          peek(void);

  char          read(void);
  char          readuntil(char stop);
  uint64        read(void *buf, uint64 len);
  uint64        read(void *buf, uint64 maxlen, char stop);

  bool          peekIFFchunk(char *name, uint32 &dataLen);


  //  Read a generic chunk into re-allocatable memory.
  void          readIFFchunk(char   *name,
                             uint8 *&data,
                             uint32 &dataLen,
                             uint32 &dataMax);

  //  Read a specific chunk into a pre-allocated buffer.
  bool          readIFFchunk(char const *name,
                             void       *data,
                             uint32      dataLen);

  //  Read a specific chunk into an array, an object (via reference) or an
  //  object (via pointer).
  template<typename OBJ>
  bool          readIFFarray(char const *name, OBJ *array, uint32 nObjects) {
    return(readIFFchunk(name, array, sizeof(OBJ) * nObjects));
  }

  template<typename OBJ>
  bool          readIFFobject(char const *name, OBJ &object) {
    return(readIFFchunk(name, &object, sizeof(OBJ)));
  }

  template<typename OBJ>
  bool          readIFFobject(char const *name, OBJ *object) {
    return(readIFFchunk(name, object, sizeof(OBJ)));
  }

  void          skipAhead(char stop, bool after=false);
  uint64        copyUntil(char stop, char *dest, uint64 destLen);

  void          seek(uint64 pos, uint64 extra=0);
  uint64        tell(void) { return(_filePos); }

  const char   *filename(void) { return(_filename); }

private:
  void          fillBuffer(void);
  void          init(int fileptr, const char *filename, uint64 bufferMax);

  char         _filename[FILENAME_MAX+1] = {0};

  int          _file     = 0;          //  Handle for file.
  uint64       _filePos  = 0;          //  Position we're at in the file.

  bool         _stdin    = false;      //  True if reading from stdin.
  bool         _eof      = false;      //  True if we hit EOF.

  bool         _ignoreCR = true;       //  Ignore blasted DOS CR letters in read() and readuntil().

  uint64       _bufferBgn = 0;         //  File position where this buffer is from.

  uint64       _bufferPos = 0;         //  Position in the buffer we're at.
  uint64       _bufferLen = 0;         //  Length of the valid data in the buffer.
  uint64       _bufferMax = 0;         //  Size of _buffer allocation.
  uint8       *_buffer    = nullptr;   //  Data!
};


//  Returns the next letter in the buffer, but DOES NOT advance past
//  it.  Might have some wierd interaction with EOF -- if you peek()
//  and the next thing is eof , the _eof flag might get set.
//
inline
char
readBuffer::peek(void) {

  if ((_eof == false) && (_bufferPos >= _bufferLen))
    fillBuffer();

  if (_eof)
    return(0);

  return(_buffer[_bufferPos]);
}


//  Returns the next letter in the buffer and advances file position by one.
//  Returns 0 on EOF.
//
inline
char
readBuffer::read(void) {

 readAgain:
  if ((_eof == false) && (_bufferPos >= _bufferLen))
    fillBuffer();

  if (_eof)
    return(0);

  _bufferPos++;
  _filePos++;

  if ((_ignoreCR) && (_buffer[_bufferPos-1] == '\r'))
    goto readAgain;

  return(_buffer[_bufferPos-1]);
}

//  Like read(), but does not advance file position if
//  the 'stop' character is encountered.
//  Returns 'stop' on EOF.
//
inline
char
readBuffer::readuntil(char stop) {
  char  ch;

 readuntilAgain:
  if ((_eof == false) && (_bufferPos >= _bufferLen))
    fillBuffer();

  if (_eof)
    return(0);

  ch = _buffer[_bufferPos];

  if (ch == stop)
    return(stop);

  _bufferPos++;
  _filePos++;

  if ((_ignoreCR) && (ch == '\r'))
    goto readuntilAgain;

  return(ch);
}

//  Advances the file position to the next 'stop' character and returns
//  'stop', or 0 if eof.
//
//  The file is left on the 'stop' character if after == false,
//  otherwise, the file is positioned after the 'stop' character.
//
inline
void
readBuffer::skipAhead(char stop, bool after) {

  while (_eof == false) {

    //  Insert a fake stop at the end of the buffer.
    _buffer[_bufferLen] = stop;

    //  Scan the buffer for the next real or fake stop.
    while (_buffer[_bufferPos] != stop) {
      _bufferPos++;
      _filePos++;
    }

    //  If we hit the fake stop, fill the buffer again and continue.
    if (_bufferPos == _bufferLen) {
      fillBuffer();
      continue;
    }

    //  Otherwise, we've found a real stop.  Skip it if desired.
    if (after) {
      _bufferPos++;
      _filePos++;
    }

    //  Fill the buffer again, if needed.
    if (_bufferPos == _bufferLen)
      fillBuffer();

    return;
  }
}

//  Advances the file position to the next 'stop' character, copying
//  the characters into a user buffer.
//
//  The file is left on the 'stop' character.
//
//  Returns the number of letters copied.
//
inline
uint64
readBuffer::copyUntil(char stop, char *dest, uint64 destLen) {
  uint64  copied = 0;

  while (_eof == false) {
    while ((_buffer[_bufferPos] != stop) &&
           (_bufferPos < _bufferLen) &&
           (copied < destLen)) {
      dest[copied] = _buffer[_bufferPos];

      copied++;
      _bufferPos++;
      _filePos++;
    }

    if (_bufferPos < _bufferLen)
      return(copied);

    fillBuffer();
  }

  return(copied);
}


}  //  merylutil::files::v1

#endif  //  MERYLUTIL_FILES_BUFFERED_V1_READING_H
