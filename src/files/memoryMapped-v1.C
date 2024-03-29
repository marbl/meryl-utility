
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
#include <sys/mman.h>

#include "files.H"

namespace merylutil::inline files::inline v1 {

memoryMappedFile::memoryMappedFile(const char *name,
                                   mftType     type) {

  strncpy(_name, name, FILENAME_MAX-1);

  _type = type;

  errno = 0;
  _fd = (_type == mftReadOnly) ? open(_name, O_RDONLY | O_LARGEFILE)
                                : open(_name, O_RDWR   | O_LARGEFILE);
  if (errno)
    fprintf(stderr, "memoryMappedFile()-- Couldn't open '%s' for mmap: %s\n", _name, strerror(errno)), exit(1);

  struct stat  sb;

  fstat(_fd, &sb);
  if (errno)
    fprintf(stderr, "memoryMappedFile()-- Couldn't stat '%s' for mmap: %s\n", _name, strerror(errno)), exit(1);

  _length = sb.st_size;
  _offset = 0;

  if (_length == 0)
    fprintf(stderr, "memoryMappedFile()-- File '%s' is empty, can't mmap.\n", _name), exit(1);

  //  Map the file to memory, or grab some anonymous space for the file to be copied to.

  if (_type == mftReadOnly)
    _data = mmap(0L, _length, PROT_READ,              MAP_FILE | MAP_PRIVATE, _fd, 0);

  if (_type == mftReadOnlyInCore)
    _data = mmap(0L, _length, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

  if (_type == mftReadWrite)
    _data = mmap(0L, _length, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, _fd, 0);

  if (_type == mftReadWriteInCore)
    _data = mmap(0L, _length, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

  //  If loading into core, read the file into core.

  if ((_type == mftReadOnlyInCore) ||
      (_type == mftReadWriteInCore))
    read(_fd, _data, _length);

  //  Close the file if we're done with it.

  if (_type != mftReadWriteInCore)
    close(_fd), _fd = -1;

  //  Catch any and all errors.

  if (errno)
    fprintf(stderr, "memoryMappedFile()-- Couldn't mmap '%s' of length " F_SIZE_T ": %s\n", _name, _length, strerror(errno)), exit(1);


  //fprintf(stderr, "memoryMappedFile()-- File '%s' of length %lu is mapped.\n", _name, _length);
}


memoryMappedFile::~memoryMappedFile() {

  errno = 0;

  if (_type == mftReadWrite)
    msync(_data, _length, MS_SYNC);

  if (_type == mftReadWriteInCore)
    write(_fd, _data, _length), close(_fd);

  if (errno)
    fprintf(stderr, "memoryMappedFile()-- Failed to close mmap '%s' of length " F_SIZE_T ": %s\n", _name, _length, strerror(errno)), exit(1);

  //  Destroy the mapping.

  munmap(_data, _length);
}

}  //  merylutil::files::v1
