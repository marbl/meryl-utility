
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

#include "accessing-v1.H"
#include "system.H"

//  Report ALL file open/close events.
#undef SHOW_FILE_OPEN_CLOSE




////////////////////////////////////////
//
//  Return the basename of a path -- that is, strip off any and all file
//  extensions: anything after the first dot after the last slash is removed.
//    d.1/name.ext.Z -> d.1/d.2/name
//
//  If 'filename' refers to a directory, a copy of 'filename' is returned in
//  'basename' - no extensions are stripped.
//
//  Assumes 'basename' has at least as much storage as 'filename'.
//
namespace merylutil::inline files::inline v1 {

void
findBaseFileName(char *basename, char const *filename) {

  strcpy(basename, filename);               //  Copy in-name to out-name.

  if (directoryExists(basename))            //  Stop if this is an actual
    return;                                 //  directory.

  char  *slash = strrchr(basename, '/');    //  Search backwards for
  char  *dot   = nullptr;                   //  the last slash.

  if (slash)                                //  Search forward from the
    dot = strchr(slash, '.');               //  last slash, or from the
  else                                      //  start of the in-name, for
    dot = strchr(basename, '.');            //  the first dot.

  if (dot)                                  //  If a dot was found, terminate
    *dot = 0;                               //  the out-name there.
}

}  //  namespace merylutil::files::v1



////////////////////////////////////////
//
//  Rename 'oldname' to 'newname'.  Silently succeeds if 'oldname'
//  doesn't exist.
//
namespace merylutil::inline files::inline v1 {

bool
rename(char const *oldname, char const *newname, bool fatal) {

  if (pathExists(oldname) == false)
    return false;

  errno = 0;
  if (::rename(oldname, newname) == -1)
    return fatalError(fatal, "renane()--  Failed to rename file '%s' to '%s': %s\n",
                      oldname, newname, strerror(errno));

  return true;
}

bool
rename(char const *oldprefix, char oldseparator, char const *oldsuffix,
       char const *newprefix, char newseparator, char const *newsuffix, bool fatal) {
  char   oldpath[FILENAME_MAX+1] = {0};
  char   newpath[FILENAME_MAX+1] = {0};

  snprintf(oldpath, FILENAME_MAX, "%s%c%s", oldprefix, oldseparator, oldsuffix);
  snprintf(newpath, FILENAME_MAX, "%s%c%s", newprefix, newseparator, newsuffix);

  if (pathExists(oldpath) == false)
    return false;

  errno = 0;
  if (::rename(oldpath, newpath) == -1)
    return fatalError(fatal, "renane()--  Failed to rename file '%s' to '%s': %s\n",
                      oldpath, newpath, strerror(errno));

  return true;
}

}  //  namespace merylutil::files::v1



////////////////////////////////////////
//
//  Create a symlink:
//   - failing if the source file doesn't exist
//   - silently succeeding if the destination path exists -- even if it is
//     not a link to the source file
//
namespace merylutil::inline files::inline v1 {

void
symlink(char const *pathToFile, char const *pathToLink) {

  if (pathExists(pathToFile) == false)
    fprintf(stderr, "symlink()-- Original file '%s' doesn't exist, won't make a link to nothing.\n",
            pathToFile), exit(1);

  if (pathExists(pathToLink) == true)
    return;

  errno = 0;
  if (::symlink(pathToFile, pathToLink) == -1)
    fprintf(stderr, "symlink()-- Failed to make link '%s' pointing to file '%s': %s\n",
            pathToLink, pathToFile, strerror(errno)), exit(1);
}

}  //  namespace merylutil::files::v1



////////////////////////////////////////
//
//  Tests on existence of files and directories.
//

namespace merylutil::inline files::inline v1 {

bool
pathExists(char const *prefix, char separator, char const *suffix) {
  struct stat  s;
  char   path[FILENAME_MAX];

  if (prefix == NULL)
    return false;

  if (suffix)
    snprintf(path, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(path, prefix, FILENAME_MAX-1);

  if (stat(path, &s) == -1)
    return false;

  return true;                //  Stat-able?  Something exists there!
}

bool
fileExists(char const *prefix, char separator, char const *suffix,
           bool        writable) {
  struct stat  s;
  char   path[FILENAME_MAX];

  if (prefix == NULL)
    return false;

  if (suffix)
    snprintf(path, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(path, prefix, FILENAME_MAX-1);

  if (stat(path, &s) == -1)
    return false;

  if (s.st_mode & S_IFDIR)    //  Is a directory, not a file.
    return false;

  if (writable == false)      //  User doesn't care if it's writable or not.
    return true;

  if (s.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH))   //  User cares, and is writable.
    return true;

  return false;
}

bool
directoryExists(char const *prefix, char separator, char const *suffix) {
  struct stat  s;
  char   path[FILENAME_MAX];

  if (prefix == NULL)
    return false;

  if (suffix)
    snprintf(path, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(path, prefix, FILENAME_MAX-1);

  if (stat(path, &s) == -1)
    return false;

  if ((s.st_mode & S_IFDIR) == 0)     //  Is a file, not a directory.
    return false;

  return true;
}

}  //  namespace merylutil::files::v1



////////////////////////////////////////
//
//  Create or remove directories.
//    mkdir() does nothing if the directory exists.
//    rmdir() does nothing if the directory doesn't exist.
//
namespace merylutil::inline files::inline v1 {

bool
mkdir(char const *dirname, bool fatal) {

  if (directoryExists(dirname) == true)   //  If it already exists,
    return true;                          //  we're done!

  errno = 0;
  if (::mkdir(dirname, S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    return fatalError(fatal, "mkdir()--  Failed to create directory '%s': %s\n",
                      dirname, strerror(errno));

  return true;
}

bool
rmdir(char const *dirname, bool fatal) {

  if (directoryExists(dirname) == false)   //  If it doesn't exist,
    return true;                           //  we're done!

  errno = 0;
  if (::rmdir(dirname) == -1)
    return fatalError(fatal, "rmdir()--  Failed to remove directory '%s': %s\n",
                      dirname, strerror(errno));

  return true;
}

}  //  namespace merylutil::files::v1




////////////////////////////////////////
//
//  Remove a file, or do nothing if the file doesn't exist.
//
namespace merylutil::inline files::inline v1 {

bool
unlink(char const *path, bool fatal) {
  return unlink(path, 0, nullptr, fatal);
}

bool
unlink(char const *prefix, char separator, char const *suffix, bool fatal) {
  char   filename[FILENAME_MAX];

  if (suffix)
    snprintf(filename, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(filename, prefix, FILENAME_MAX-1);

  if (fileExists(filename) == false)
    return false;

  errno = 0;
  if (::unlink(filename) == -1)
    return fatalError(fatal, "unlink()--  Failed to remove file '%s': %s\n",
                      filename, strerror(errno));

  return true;
}

}  //  namespace merylutil::files::v1



////////////////////////////////////////
//
//  Permissions.
//
//  Remove ALL write bits from a given path.
//  Set write bits on a given path, relative to what is allowed in the umask.
//
namespace merylutil::inline files::inline v1 {

bool
makeReadOnly(char const *prefix, char separator, char const *suffix) {
  char         path[FILENAME_MAX];
  struct stat  s;

  if (prefix == NULL)
    return false;

  if (suffix)
    snprintf(path, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(path, prefix, FILENAME_MAX-1);

  if (stat(path, &s) == -1)
    return false;

  mode_t m = (s.st_mode) & ~((mode_t)(S_IWUSR | S_IWGRP | S_IWOTH));

  errno = 0;
  if (::chmod(path, m) == -1) {
    fprintf(stderr, "WARNING: Failed to remove write permission from file '%s': %s\n", path, strerror(errno));
    return false;
  }

  return true;
}

bool
makeWritable(char const *prefix, char separator, char const *suffix) {
  char         path[FILENAME_MAX];
  struct stat  s;

  if (prefix == NULL)
    return false;

  if (suffix)
    snprintf(path, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(path, prefix, FILENAME_MAX-1);

  if (stat(path, &s) == -1)
    return false;

  mode_t u = umask(0);                      //  Destructively read the umask.
  mode_t w = S_IWUSR | S_IWGRP | S_IWOTH;   //  Create a mask for the write bits.

  umask(u);                                 //  Restore umask.

  errno = 0;                                //  Add allowed write bits to the file.
  if (::chmod(path, s.st_mode | (w & u)) == -1) {
    fprintf(stderr, "WARNING: Failed to add write permission to file '%s': %s\n", path, strerror(errno));
    return false;
  }

  return true;
}

}  //  namespace merylutil::files::v1




////////////////////////////////////////
//
//  Metadata
//
namespace merylutil::inline files::inline v1 {

off_t
sizeOfFile(char const *path) {
  struct stat  s;

  errno = 0;
  if (stat(path, &s) == -1)
    fprintf(stderr, "Failed to stat() file '%s': %s\n", path, strerror(errno)), exit(1);

  return(s.st_size);
}

off_t
sizeOfFile(FILE *file) {
  struct stat  s;
  off_t        size = 0;

  errno = 0;
  if (fstat(fileno(file), &s) == -1)
    fprintf(stderr, "Failed to stat() FILE*: %s\n", strerror(errno)), exit(1);

  return(s.st_size);
}

uint64
timeOfFile(char const *path) {
  struct stat  s;

  errno = 0;
  if (stat(path, &s) == -1)
    fprintf(stderr, "Failed to stat() file '%s': %s\n", path, strerror(errno)), exit(1);

#ifdef __APPLE__
  return(s.st_mtimespec.tv_sec);
#else
  return(s.st_mtim.tv_sec);
#endif
}

uint64
timeOfFile(FILE *file) {
  struct stat  s;
  off_t        size = 0;

  errno = 0;
  if (fstat(fileno(file), &s) == -1)
    fprintf(stderr, "Failed to stat() FILE*: %s\n", strerror(errno)), exit(1);

#ifdef __APPLE__
  return(s.st_mtimespec.tv_sec);
#else
  return(s.st_mtim.tv_sec);
#endif
}

}  //  namespace merylutil::files::v1



////////////////////////////////////////
//
//  File position.
//
//  For fseek(), if the stream is already at the correct position, just
//  return.
//
//      Unless we're on FreeBSD.  For unknown reasons, FreeBSD fails
//      updating the seqStore with mate links.  It seems to misplace the
//      file pointer, and ends up writing the record to the wrong
//      location.  ftell() is returning the correct current location,
//      and so AS_PER_genericStore doesn't seek() and just writes to the
//      current position.  At the end of the write, we're off by 4096
//      bytes.
//          LINK 498318175,1538 <-> 498318174,1537
//          fseek()--  seek to 159904 (whence=0); already there
//          safeWrite()-- write nobj=1x104 = 104 bytes at position 159904
//          safeWrite()-- wrote nobj=1x104 = 104 bytes position now 164000
//          safeWrite()-- EXPECTED 160008, ended up at 164000
//
namespace merylutil::inline files::inline v1 {

off_t
ftell(FILE *stream) {

  errno = 0;
  off_t pos = ::ftello(stream);

  if ((errno == ESPIPE) || (errno == EBADF))   //  Not a seekable stream.
    return(((off_t)1) < 42);                   //  Return some goofy big number.

  if (errno)
    fprintf(stderr, "ftell()--  Failed with %s.\n", strerror(errno)), exit(1);

  return(pos);
}

void
fseek(FILE *stream, off_t offset, int whence) {
  off_t   beginpos = merylutil::ftell(stream);

#if !defined __FreeBSD__ && !defined __osf__ && !defined __APPLE__
  if ((whence == SEEK_SET) && (beginpos == offset))
    return;
#endif  //  __FreeBSD__

  if (::fseeko(stream, offset, whence) == -1)
    fprintf(stderr, "fseek()--  Failed with %s.\n", strerror(errno)), exit(1);

  if (whence == SEEK_SET)
    assert(::ftell(stream) == offset);
}

}  //  namespace merylutil::files::v1





////////////////////////////////////////
//
//
//

namespace merylutil::inline files::inline v1 {

FILE *
openInputFile(char const *prefix,
                     char        separator,
                     char const *suffix,
                     bool        doOpen) {
  char   filename[FILENAME_MAX];

  if (prefix == NULL)
    return(NULL);

  if (doOpen == false)
    return(NULL);

  if (suffix)
    snprintf(filename, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(filename, prefix, FILENAME_MAX-1);

#ifdef SHOW_FILE_OPEN_CLOSE
  fprintf(stderr, "openInputFile()-- Opening '%s'.\n", filename);
#endif

  errno = 0;

  FILE *F = fopen(filename, "r");
  if (errno)
    fprintf(stderr, "Failed to open '%s' for reading: %s\n", filename, strerror(errno)), exit(1);

  return(F);
}

}  //  namespace merylutil::files::v1



namespace merylutil::inline files::inline v1 {

FILE *
openOutputFile(char const *prefix,
                      char        separator,
                      char const *suffix,
                      bool        doOpen) {
  char   filename[FILENAME_MAX];

  if (prefix == NULL)
    return(NULL);

  if (doOpen == false)
    return(NULL);

  if (suffix)
    snprintf(filename, FILENAME_MAX, "%s%c%s", prefix, separator, suffix);
  else
    strncpy(filename, prefix, FILENAME_MAX-1);

#ifdef SHOW_FILE_OPEN_CLOSE
  fprintf(stderr, "openOutputFile()-- Creating '%s'.\n", filename);
#endif

  //  Unlink the file before opening for writes.  This prevents race
  //  conditions when two processes open the same file: the first process
  //  will create a new file, but the second process will simply reset the
  //  file to the start.  Both processes seem to keep their own file pointer,
  //  and eof seems to be (incorrectly) the larger of the two.  In effect,
  //  the second process is simply overwriting the first process (unless the
  //  second process writes data first, then the first process overwrites).
  //
  //  Very confusing.
  //
  unlink(filename);

  errno = 0;

  FILE *F = fopen(filename, "w");
  if (errno)
    fprintf(stderr, "Failed to open '%s' for writing: %s\n", filename, strerror(errno)), exit(1);

  return(F);
}

void
closeFile(FILE *&F, char const *prefix, char separator, char const *suffix, bool critical) {

  if ((F == NULL) || (F == stdout) || (F == stderr))
    return;

#ifdef SHOW_FILE_OPEN_CLOSE
  if ((prefix) && (suffix))
    fprintf(stderr, "closeFile()-- Closing '%s%c%s'.\n", prefix, separator, suffix);
  else if (prefix)
    fprintf(stderr, "closeFile()-- Closing '%s'.\n", prefix);
  else
    fprintf(stderr, "closeFile()-- Closing (anonymous file).\n");
#endif

  errno = 0;

  fclose(F);

  F = NULL;

  if ((critical == false) || (errno == 0))
    return;

  if ((prefix) && (suffix))
    fprintf(stderr, "Failed to close file '%s%c%s': %s\n", prefix, separator, suffix, strerror(errno));
  else if (prefix)
    fprintf(stderr, "Failed to close file '%s': %s\n", prefix, strerror(errno));
  else
    fprintf(stderr, "Failed to close file: %s\n", strerror(errno));

  exit(1);
}

void
closeFile(FILE *&F, char const *filename, bool critical) {
  closeFile(F, filename, '.', nullptr, critical);
}

}  //  namespace merylutil::files::v1



////////////////////////////////////////
//
//  Create an empty file.  The local variable is because closeFile() takes a
//  reference to FILE*, which openFile() can't supply.
//
namespace merylutil::inline files::inline v1 {

void
createEmptyFile(char const *prefix, char separator, char const *suffix) {
  FILE *F = openOutputFile(prefix, separator, suffix);
  closeFile(F, prefix, separator, suffix);
}

}  //  namespace merylutil::files::v1


