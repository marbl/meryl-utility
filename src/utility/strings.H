
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

#ifndef STRINGS_H
#define STRINGS_H

#include "types.H"

#include <set>
#include <vector>


//  Some string cleanup functions.
//

void   chomp(char *S);    //  Remove whitespace from the end of a line.


//  Basic string functions.

inline
bool
isEmptyString(char const *s)   { return((s == nullptr) || (s[0] == 0)); }

//  Convert a line into a key-value pair.
//
//  The line should be of the form:
//   - 'key'           find() returns true with value == nullptr
//   - 'key = value'   find() returns true
//   - 'key : value'   find() returns true
//
//  In all cases the line is modified by removing whitespace and inserting
//  NUL characters at the end of the key and value.
//
//  Comments are accepted with either '!' or '#' at the start
//  of the line, or preceeded by a white-space letter.

class KeyAndValue {
public:
  KeyAndValue(const char *line = nullptr)   {  find(line);       };
  ~KeyAndValue()                            {  delete [] _line;  };

public:
  bool     find(const char *line);

public:
  char    *key(void)           { return(_key); };
  char    *value(void)         { return(_val); };

private:
  uint32  _lineMax = 0;
  uint32  _lineLen = 0;
  char   *_line    = nullptr;

  char   *_key     = nullptr;
  char   *_val     = nullptr;
};

//  Split the input 'line' into an array of words or path
//  components.

enum splitType {
  splitWords      = 0,   //  Split on whitespace
  splitPaths      = 1,   //  Split on '/'                (not valid for stringList)
  splitAsIs       = 2,   //  Don't modify _sc, use as is (not valid for stringList)

  splitWhitespace = 3,   //  Split on whitespace
  splitTabs       = 4,   //  Split on tabs
  splitLines      = 5    //  Split on new line (and carriage return)
};

class splitToWords {
public:
  splitToWords(const char *string=nullptr, splitType type=splitWords) { split(string, type); };
  splitToWords(const char *string,         char sep)                  { split(string, sep);  };
  splitToWords(const char *string,         char const *sep)           { split(string, sep);  };

  ~splitToWords() {
    erase();
  };

  void    split(const char *line, splitType type=splitWords);
  void    split(const char *line, char        sep);
  void    split(const char *line, char const *sep);

  void    clear(void);   //  Remove the words, but leave memory intact.
  void    erase(void);   //  Remove words and free memory.

private:
  void   clearsc(void) {
    _sc[0] = _sc[1] = _sc[2] = _sc[3] = 0;
  }

  void   setsc(uint8 c) {
    _sc[(c & 0xc0) >> 6] |= (uint64one << (c & 0x3f));
  }

  bool   issc(uint8 c) {
    return(_sc[(c & 0xc0) >> 6] >> (c & 0x3f) & 0x01);
  };

public:
  uint32  numWords(void)        { return(_wordsLen); };

  char   *operator[](uint32 i)  { return(first(i)); };

  char   *first(uint32 i=0)     { return((_wordsLen <= i) ? nullptr : _words[i]);  };
  char   *last(uint32 i=0)      { return((_wordsLen == 0) ? nullptr : _words[_wordsLen - i - 1]); };
  char   *pop(void)             { return((_wordsLen == 0) ? nullptr : _words[--_wordsLen]);       };
  char   *shift(void)           {
    if (_wordsLen == 0)                     //  If no words, nothing to return.
      return(nullptr);

    for (uint32 ii=1; ii<_wordsLen; ii++)   //  Shift all words down one place, moving
      std::swap(_words[ii-1], _words[ii]);  //  the word to shift off to the end.

    return(_words[--_wordsLen]);            //  Return the word we shifted out.
  };

  int32   toint32(uint32 i)     { return(strtoint32 (_words[i])); };
  uint32  touint32(uint32 i)    { return(strtouint32(_words[i])); };
  int64   toint64(uint32 i)     { return(strtoint64 (_words[i])); };
  uint64  touint64(uint32 i)    { return(strtouint64(_words[i])); };
  double  todouble(uint32 i)    { return(strtodouble(_words[i])); };

private:
  uint64    _sc[4]    = {0};       //  Bit array of letters to split on.

  uint32    _wordsLen = 0;         //  An array of pointers into _chars
  uint32    _wordsMax = 0;         //  for the words in the string.
  char    **_words    = nullptr;

  uint32    _charsLen = 0;         //  A modified copy of the
  uint32    _charsMax = 0;         //  input string.
  char     *_chars    = nullptr;
};



////////////////////////////////////////
//
//  stringList - efficiently store a collection of strings.
//
//  The intended purpose is to load a list of words read, one per line, from
//  a file.  Extended to support breaking at tabs or generic white space.
//  However, empty records are ignored - in tab mode, 'a b\t\tc\td' will
//  return strings 'a b', 'c', 'd'; the two tabs between the first two words
//  is treated as one separator.
//  
class stringList {
public:
  stringList()                                              {                      };
  stringList(char const *filename, splitType st=splitWords) {  load(filename, st); };

  ~stringList() {
    for (uint64 ii=0; ii<_data.size(); ii++)
      delete [] _data[ii];
  }

  void            load(char const *filename, splitType st=splitLines);

  uint64          size(void) {
    return(_pointers.size());
  };

  char const *    operator[](uint64 ii) {
    if (ii < _pointers.size())
      return(_pointers[ii]);
    else
      return(nullptr);
  }

private:
  std::vector<char const *>  _pointers;
  std::vector<char const *>  _data;
};

#endif  //  STRINGS_H
