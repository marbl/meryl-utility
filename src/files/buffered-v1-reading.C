
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

#include <fcntl.h>

#include "arrays.H"
#include "files.H"

namespace merylutil::inline files::inline v1 {


void
readBuffer::initialize(const char *pfx, char sep, const char *sfx, uint64 bMax) {

  //  Figure out what filename to read from.
  //    pfx == nullptr                    -> stdin
  //    pfx == '-'     and sfx == nullptr -> stdin
  //    pfx != nullptr and sfx == nullptr -> {pfx}
  //    pfx != nullptr and sfx != nullptr -> {pfx}{sep}{sfx}
  //
  //  A sep of 0 should probably also result in just {pfx}, but it is currently allowed.

  if ((pfx == nullptr) ||
      ((pfx[0] == '-') && (pfx[1] == 0) && (sfx == nullptr))) {
    strcpy(_filename, "(stdin)");
    _stdin = true;
  }
  else if ((pfx != nullptr) && (sfx == nullptr)) {
    strncpy(_filename, pfx, FILENAME_MAX);
    _owned = true;
  }
  else if ((pfx != nullptr) && (sfx != nullptr)) {
    snprintf(_filename, FILENAME_MAX, "%s%c%s", pfx, sep, sfx);
    _owned = true;
  }
  else {
    fprintf(stderr, "Invalid readBuffer filename pfx='%s' sep='%d' sfx='%s'\n", pfx, sep, sfx);
    exit(1);
  }

  //  Allocate a buffer.

  _bufferMax   = (bMax == 0) ? 32 * 1024 : bMax;
  _buffer      = new uint8 [_bufferMax + 1];

  //  Open the file, failing if it's actually the terminal.

  errno = 0;
  _file = (_stdin) ? fileno(stdin) : open(_filename, O_RDONLY | O_LARGEFILE);
  if (_file == -1)
    fprintf(stderr, "readBuffer()-- couldn't open file '%s': %s\n",
            _filename, strerror(errno)), exit(1);

  if (isatty(_file)) {
    fprintf(stderr, "readBuffer()-- cannot use the terminal for input; provide a filename or input from a pipe.\n");
    exit(1);
  }

  //  Fill the buffer.

  fillBuffer();
}



readBuffer::readBuffer(FILE *file, uint64 bufferMax) {

  strcpy(_filename, "(hidden file)");

  //  Allocate a buffer.

  _bufferMax   = (bufferMax == 0) ? 32 * 1024 : bufferMax;
  _buffer      = new uint8 [_bufferMax + 1];

  //  Open the file.  It never fails.

  _file        = fileno(file);

  //  Rewind the file (allowing failure if it's a pipe or stdin).

  errno = 0;
  if ((lseek(_file, 0, SEEK_SET) == -1) && (errno != ESPIPE))
    fprintf(stderr, "readBuffer()-- '%s' couldn't seek to position 0: %s\n",
            _filename, strerror(errno)), exit(1);

  //  Fill the buffer.

  fillBuffer();
}



readBuffer::~readBuffer() {

  delete [] _buffer;

  if (_owned == true)   //  Close the file if we opened it.
    close(_file);
}



void
readBuffer::fillBuffer(void) {

  //  If there is still stuff in the buffer, no need to fill.

  if (_bufferPos < _bufferLen)
    return;

  _bufferBgn += _bufferLen;

  _bufferPos  = 0;
  _bufferLen  = 0;

  assert(_filePos == _bufferBgn);

 again:
  errno = 0;
  _bufferLen = (uint64)::read(_file, _buffer, _bufferMax);

  if (errno == EAGAIN)
    goto again;

  if (errno)
    fprintf(stderr, "readBuffer::fillBuffer()-- only read " F_U64 " bytes, couldn't read " F_U64 " bytes from '%s': %s\n",
            _bufferLen, _bufferMax, _filename, strerror(errno)), exit(1);

  if (_bufferLen == 0)
    _eof = true;
}



void
readBuffer::seek(uint64 pos, uint64 extra) {

  //  If not really a seek, and the buffer still has enough stuff in it, just return.

  if ((pos == _filePos) && (_filePos + extra < _bufferBgn + _bufferLen))
    return;

  //  If stdin, we can't seek.

  if (_stdin == true) {
    fprintf(stderr, "readBuffer()-- seek() not available for file 'stdin'.\n");
    exit(1);
  }

  //  If the position is in the buffer, just move there and
  //  potentially skip any actual file access.

  else if ((pos < _filePos) &&
           (_bufferBgn  <= pos) &&
           (pos + extra <  _bufferBgn + _bufferLen)) {
    if (pos < _filePos) {
      //fprintf(stderr, "readBuffer::seek()-- jump back to position %lu from position %lu (buffer at %lu)\n",
      //        pos, _filePos, _bufferPos);
      _bufferPos -= (_filePos - pos);
      _filePos   -= (_filePos - pos);
    } else {
      //fprintf(stderr, "readBuffer::seek()-- jump ahead to position %lu from position %lu (buffer at %lu)\n",
      //        pos, _filePos, _bufferPos);
      _bufferPos += (pos - _filePos);
      _filePos   += (pos - _filePos);
    }
  }

  //  Nope, we need to grab a new block of data.

  else {
    //fprintf(stderr, "readBuffer::seek()-- jump directly to position %lu from position %lu (buffer at %lu)\n",
    //        pos, _filePos, _bufferPos);

    errno = 0;
    lseek(_file, pos, SEEK_SET);
    if (errno)
      fprintf(stderr, "readBuffer()-- '%s' couldn't seek to position " F_U64 ": %s\n",
              _filename, pos, strerror(errno)), exit(1);

    _filePos   = pos;

    _bufferBgn = pos;
    _bufferLen = 0;

    _bufferPos = 0;

    fillBuffer();
  }

  _eof = (_bufferPos >= _bufferLen);
}



uint64
readBuffer::read(void *buf, uint64 len) {
  char  *bufchar = (char *)buf;

  //  Easy case; the next len bytes are already in the buffer; just
  //  copy and move the position.

  if (_bufferPos + len <= _bufferLen) {
    memcpy(bufchar, _buffer + _bufferPos, len);

    _filePos   += len;
    _bufferPos += len;

    fillBuffer();

    return(len);
  }

  //  Existing buffer not big enough.  Copy what's there, then finish
  //  with a read.

  uint64   bCopied = 0;   //  Number of bytes copied into the buffer
  uint64   bAct    = 0;   //  Number of bytes actually read from disk

  bCopied     = _bufferLen - _bufferPos;

  memcpy(bufchar, _buffer + _bufferPos, bCopied);

  while (bCopied < len) {
    errno = 0;
    bAct = (uint64)::read(_file, bufchar + bCopied, len - bCopied);
    if (errno)
      fprintf(stderr, "readBuffer()-- couldn't read " F_U64 " bytes from '%s': n%s\n",
              len, _filename, strerror(errno)), exit(1);

    if (bAct == 0)    //  If we hit EOF, return a short read.
      len = 0;

    bCopied += bAct;
  }

  _filePos   += bCopied;    //  Advance the actual file position to however much we just read.

  _bufferBgn  = _filePos;   //  And set the buffer begin to that too.
  _bufferLen  = 0;          //  Set the buffer as empty, so we fill it again.

  _bufferPos  = 0;

  fillBuffer();

  return(bCopied);
}



uint64
readBuffer::read(void *buf, uint64 maxlen, char stop) {
  char  *bufchar = (char *)buf;
  uint64 c = 0;

  //  We will copy up to 'maxlen'-1 bytes into 'buf', or stop at the first occurrence of 'stop'.
  //  This will reserve space at the end of any string for a zero-terminating byte.
  maxlen--;

  while ((_eof == false) && (c < maxlen)) {
    bufchar[c++] = _buffer[_bufferPos];

    _filePos++;
    _bufferPos++;

    if (_bufferPos >= _bufferLen)
      fillBuffer();

    if (bufchar[c-1] == stop)
      break;
  }

  bufchar[c] = 0;

  return(c);
}



bool
readBuffer::peekIFFchunk(char name[4], uint32 &dataLen) {

  //  Seek to the current position, making sure there are at least
  //  8 bytes still in the buffer.

  seek(_filePos, 8);

  //  If there's space for a valid IFF header, return the name and length.

  if (_bufferPos + 8 < _bufferLen) {
    memcpy( name,    _buffer + _bufferPos,     sizeof(char) * 4);
    memcpy(&dataLen, _buffer + _bufferPos + 4, sizeof(uint32));

    return(true);
  }

  //  If not, return an empty name and length of zero.

  name[0] = 0;
  name[1] = 0;
  name[2] = 0;
  name[3] = 0;

  dataLen = 0;

  return(false);
}



//  Read a specific chunk from the buffer.
//  Return false if the chunk isn't named 'name' and of length 'dataLen'.
//
bool
readBuffer::readIFFchunk(char const *name,
                         void       *data,
                         uint32      dataLen) {
  char    dtag[4] = {0};
  uint32  dlen    =  0;

  if (peekIFFchunk(dtag, dlen) == false)
    return(false);

  if ((dtag[0] != name[0]) ||
      (dtag[1] != name[1]) ||
      (dtag[2] != name[2]) ||
      (dtag[3] != name[3]) ||
      (dlen    != dataLen))
    return(false);

  //  It's the one we want, so read the data for real.

  uint32   rl = 0;

  rl += read( dtag, 4);
  rl += read(&dlen, sizeof(uint32));
  rl += read( data, dataLen);

  return(rl == 4 + sizeof(uint32) + dataLen);
}



void
readBuffer::readIFFchunk(char*name, uint8 *&data, uint32 &dataLen, uint32 &dataMax) {

  //  Read the name and data length.

  read( name,    4);
  read(&dataLen, sizeof(uint32));

  //  Allocate space for the data.

  resizeArray(data, 0, dataMax, dataLen);

  //  Copy the data to 'data'.

  read(data, dataLen);
}




}  //  merylutil::files::v1

