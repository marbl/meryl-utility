
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

#ifndef MERYLUTIL_DS_SPLITTOWORDS_V1_H
#define MERYLUTIL_DS_SPLITTOWORDS_V1_H

#include "types.H"

//  Split the input 'line' into an array of words or path
//  components.

namespace merylutil::inline strings::inline v1 {

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
  void    split(const char *line, char        sep)  { clearsc(); setsc(sep); split(line, splitAsIs); };
  void    split(const char *line, char const *sep)  { clearsc(); setsc(sep); split(line, splitAsIs); };

  void    clear(void);   //  Remove the words, but leave memory intact.
  void    erase(void);   //  Remove words and free memory.

private:
  void   clearsc(void) {
    _sc[0] = _sc[1] = _sc[2] = _sc[3] = 0;
  }

  void   setsc(uint8 c) {
    _sc[(c & 0xc0) >> 6] |= (uint64one << (c & 0x3f));
  }

  void   setsc(char const *sep) {
    for (uint32 ii=0; sep[ii]; ii++)
      setsc(sep[ii]);
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

}  //  merylutil::strings::v1

#endif  //  MERYLUTIL_DS_SPLITTOWORDS_V1_H
