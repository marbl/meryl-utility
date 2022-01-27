
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

#include "strings.H"
#include "arrays.H"
#include "files.H"

////////////////////////////////////////////////////////////
//
//  Strip whitespace from the end of a line.
//

void
chomp(char *S) {
  char *t = S;

  while (*t != 0)
    t++;

  t--;

  while ((t >= S) && (isWhiteSpace(*t) == true))
    *t-- = 0;
}




////////////////////////////////////////////////////////////
//
//  Convert a line into a key=value pair.
//

bool
KeyAndValue::find(const char *line) {
  char  *ptr = nullptr;

  //  Reset our state, but return fail if there is no line.

  _key = nullptr;
  _val = nullptr;

  if (isEmptyString(line) == true)
    return(false);

  //  Copy the string so we can do bad things to it.

  duplicateArray(_line, _lineLen, _lineMax, line, (uint32)strlen(line) + 1);

  //  Zip ahead until the first non-space letter.
  //
  //  If the letter is a comment or the delimiter, we're done; there is no key.

  ptr = _line;

  while (isWhiteSpace(*ptr) == true)          //  Spaces before the key.
    ptr++;

  if ((*ptr == 0) ||
      (isComment(*ptr) == true) ||
      (isDelimiter(*ptr) == true))
    return(false);

  _key = ptr;

  //  Keep zipping ahead until the end of the line.
  //    Detect the first comment mark that is preceeded by a space.
  //      Change it to NUL to terminate the string and return.
  //
  //    Detect the first key=value delimiter.
  //      Change it to a space so we can iterate over it.
  //      Change all consecutive delimiters to a space too.

  char *equals    = nullptr;
  char *eol       = nullptr;
  bool  lastspace = false;

  while (1) {
    eol = ptr;

    if ((lastspace == true) && (isComment(*ptr) == true)) {
      *ptr = 0;
      break;
    }

    lastspace = isWhiteSpace(*ptr);

    if ((isDelimiter(*ptr) == true) && (equals == nullptr)) {
      equals = ptr;    //  Remember the first letter in the delimiter

      while ((*ptr != 0) && (isDelimiter(*ptr) == true))
        *ptr++ = ' ';

      ptr--;           //  Back up to the last delimiter letter.

      eol    = ptr;    //  Update eol since we possibly moved ptr ahead.
    }

    if (*ptr == 0)
      break;

    ptr++;
  }

  //  If no delimiter, we're done.  There cannot be a key/value pair.

  if (equals == nullptr)
    return(false);

  //  Cleanup 1:  Find the last letter in the key make the key stop there.

  while (isWhiteSpace(*equals) == true)
    equals--;

  equals++;      //  Move from the last letter of the key.
  *equals = 0;   //  Terminate the key string.
  equals++;      //  Move to the next letter, either space or the value.

  //  Cleanup 2: Find the first letter of the value.
  //  If we're at eol now, return true with an empty value string.

  while (isWhiteSpace(*equals) == true)
    equals++;

  _val = equals;

  if (equals == eol)
    return(true);

  //  Cleanup 3: Find the last letter of the value and make the value stop
  //  there.

  assert(*eol == 0);

  eol--;

  while (isWhiteSpace(*eol) == true) {
    *eol = 0;
    eol--;
  }

  return(true);
}



////////////////////////////////////////////////////////////
//
//  Split the input 'line' into an array of words or path
//  components.

void
splitToWords::split(const char *line, char sep) {

  clearsc();
  setsc(sep);

  split(line, splitAsIs);
}

void
splitToWords::split(const char *line, char const *sep) {

  clearsc();
  for (uint32 ii=0; sep[ii]; ii++)
    setsc(sep[ii]);

  split(line, splitAsIs);
}


void
splitToWords::split(const char *line, splitType type) {

  //  Initialize to no words and no characters.
  //  Then return if the input line is empty.

  _wordsLen = 0;
  _charsLen = 0;

  if (isEmptyString(line) == true)
    return;

  //  Initialize the separator array based on splitType, if needed.

  if ((type == splitWords) ||
      (type == splitWhitespace)) {
    clearsc();
    setsc(' ');
    setsc('\t');
    setsc('\n');
    setsc('\r');
  }

  if (type == splitPaths) {
    clearsc();
    setsc('/');
  }

  if (type == splitTabs) {
    clearsc();
    setsc('\t');
  }

  if (type == splitLines) {
    clearsc();
    setsc('\n');
    setsc('\r');
  }

  //  Count the number of words and chars in the input line, then make
  //  sure there is space for us to store them.

  while (line[_charsLen] != 0)
    if (issc(line[_charsLen++]))
      _wordsLen++;

  resizeArray(_words, 0, _wordsMax, _wordsLen + 1);
  resizeArray(_chars, 0, _charsMax, _charsLen + 1);

  //  Clear all the words pointers, and copy the input line to our storage.
  //  This greatly simplifies the loop, as we don't need to worry about
  //  terminating the final word.

  memset(_words, 0,    sizeof(char *) * (_wordsLen + 1));
  memcpy(_chars, line, sizeof(char)   * (_charsLen + 1));

  //  Scan the line copy, converting word separators to NUL bytes.
  //  counting and saving the start of each word in _words.

  _wordsLen = 0;

  for (uint32 st=1, ii=0; ii < _charsLen; ii++) {
    if (issc(line[ii])) {                     //  If the character is a word
      _chars[ii] = 0;                         //  separator, convert to NUL,
      st         = true;                      //  and flag the next character
    }                                         //  as the start of a new word.

    else if (st) {                            //  Otherwise, if this is the
      _words[_wordsLen++] = _chars + ii;      //  start of a word, make
      st                  = false;            //  a new word.
    }
  }
}


void
splitToWords::clear(void) {
  _wordsLen = 0;
  _charsLen = 0;
}


void
splitToWords::erase(void) {

  delete [] _words;
  delete [] _chars;

  _wordsLen = 0;
  _wordsMax = 0;
  _words    = nullptr;

  _charsLen = 0;
  _charsMax = 0;
  _chars    = nullptr;
}




////////////////////////////////////////
//
//  stringList - efficiently store a collection of strings.
//

static
bool
loadMore(char *&chunk, uint32 &cMax, uint32 &cLen, uint32 &cPos,
         compressedFileReader *file) {

  char *nhunk = new char [cMax];                //  Allocate another chunk of data.

  for (uint32 ii=0; ii<cLen - cPos; ii++)       //  Move unused bytes from the end
    nhunk[ii] = chunk[cPos + ii];               //  of the chunk to the new chunk.

  cLen -= cPos;                                 //  Reset cLen and cPos to show
  cPos -= cPos;                                 //  the moved data.

  chunk = nhunk;                                //  Save the new chunk.

  if (cMax - cLen < cMax / 2)                   //  Grab more space if the moved
    resizeArray(chunk, cLen, cMax, 2 * cMax);   //  data is big.

  cLen += loadFromFile(chunk + cLen,            //  Load more data.
                       "stringListData",
                       1, cMax - cLen, file->file(), false);

  if (cLen == 0) {                              //  Return true if there is
    delete [] chunk;                            //  data in the chunk,
    chunk = nullptr;                            //  or delete the chunk we
    return(false);                              //  just allocated.
  } else {
    return(true);
  }
}



void
stringList::load(char const *filename, splitType st) {
  uint32                 cMax  = 16;
  uint32                 cLen  = 0;
  uint32                 cPos  = 0;
  compressedFileReader  *file  = new compressedFileReader(filename);
  char                  *chunk = nullptr;

  bool   splitWS  = (st == splitWords || st == splitWhitespace);
  bool   splitTAB = (st == splitTabs);
  bool   splitEOL = (st == splitLines);

  while (loadMore(chunk, cMax, cLen, cPos, file)) {

    _data.push_back(chunk);

    //  Skip whitespace at the start of the chunk.  Usually only occurs on
    //  the first chunk load, but can occur if we're lucky enough to find
    //  the end-of-string before the last chunk ended.

    while ((cPos < cLen) &&
           (isWhiteSpace(chunk[cPos]) == true))
      cPos++;

    //  Search ahead until the next separator - either generic whitespace
    //  or newline/linefeed.
    //    cPos is the start of the next string to load
    //    sEnd is the end of the string
    //
    for (uint32 sEnd=cPos; (sEnd < cLen); sEnd++) {
      if ((splitWS  && isWhiteSpace(chunk[sEnd])) ||
          (splitTAB && isTab       (chunk[sEnd])) ||
          (splitEOL && isEndOfLine (chunk[sEnd]))) {

        chunk[sEnd] = 0;                              //  Terminate string and
        _pointers.push_back(chunk + cPos);            //  save the pointer to it.

        cPos = sEnd + 1;                              //  Advance to the next letter.

        while ((cPos < cLen) &&                       //  Skip whitespace to find
               (isWhiteSpace(chunk[cPos]) == true))   //  the next string start.
          cPos++;

        //  Update loop iterator to the last letter we processed, cPos, and
        //  let the next iteration process the letter after the first.
        sEnd = cPos;
      }
    }

    //  We finished processing all strings in the block.  Leave cPos as is,
    //  instead of updating to sEnd, so that we'll save (and process again)
    //  all bytes from the start of the current string.  The next iteration
    //  of the while() will loadMore() for us.
  }

  delete file;
}
