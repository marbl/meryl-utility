
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

using namespace std;


//  Remove whitespace from the end of a string.
inline
void
chomp(char *S) {
  char *t = S;

  while (*t != 0)
    t++;

  t--;

  while ((t >= S) && (isspace(*t) == true))
    *t-- = 0;
}

//  Advance the string pointer to the next non-space character.
inline
void
munch(char *&S) {
  while ((*S != 0) && (isspace(*S) == true))
    S++;
}

//  Advance the string pointer to the next space character.
inline
void
crunch(char *&S) {
  while ((*S != 0) && (isspace(*S) == false))
    S++;
}

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

  bool     find(const char *line);

  char    *key(void)           { return(_key); };
  char    *value(void)         { return(_val); };

#if 0
  bool     value_bool(void)    { return(strtobool(_val)); };

  int32    value_int32(void)   { return(strtoint32(_val)); };
  int64    value_int64(void)   { return(strtoint64(_val)); };

  uint32   value_uint32(void)  { return(strtouint32(_val)); };
  uint64   value_uint64(void)  { return(strtouint64(_val)); };

  float    value_float(void)   { return(strtofloat (_val)); };
  double   value_double(void)  { return(strtodouble(_val)); };
#endif

public:
  bool    iscomment(char c)    { return((c == '!') || (c == '#') || (c == 0)); };
  bool    isdelimiter(char c)  { return((c == ':') || (c == '=') || isspace(c)); };

  uint32  _lineMax = 0;
  uint32  _lineLen = 0;
  char   *_line    = nullptr;

  char   *_key     = nullptr;
  char   *_val     = nullptr;
};

//  Split the input 'line' into an array of words or path
//  components.

enum splitType {
  splitWords  = 0,
  splitPaths  = 1
};

class splitToWords {
public:
  splitToWords(const char *string=nullptr, splitType type=splitWords) {
    split(string, type);
  };

  ~splitToWords() {
    delete [] _chars;
    delete [] _words;
  } ;

  void    split(const char *line, splitType type=splitWords);

private:
  bool   isPath(char c) {
    return(c == '/');
  };

  bool   isSpace(char c) {
    return((c == ' ')  ||
           (c == '\t') ||
           (c == '\n') ||
           (c == '\r'));
  };

  bool   isSeparator(char c, splitType type) {
    return(((type == splitWords) && (isSpace(c))) ||
           ((type == splitPaths) && (isPath(c))));
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
      swap(_words[ii-1], _words[ii]);       //  the word to shift off to the end.

    return(_words[--_wordsLen]);            //  Return the word we shifted out.
  };

  int32   toint32(uint32 i)     { return(strtoint32 (_words[i])); };
  uint32  touint32(uint32 i)    { return(strtouint32(_words[i])); };
  int64   toint64(uint32 i)     { return(strtoint64 (_words[i])); };
  uint64  touint64(uint32 i)    { return(strtouint64(_words[i])); };
  double  todouble(uint32 i)    { return(strtodouble(_words[i])); };

private:
  uint32    _wordsLen = 0;         //  An array of pointers into _chars
  uint32    _wordsMax = 0;         //  for the words in the string.
  char    **_words    = nullptr;

  uint32    _charsLen = 0;         //  A modified copy of the
  uint32    _charsMax = 0;         //  input string.
  char     *_chars    = nullptr;
};







#endif  //  STRINGS_H
