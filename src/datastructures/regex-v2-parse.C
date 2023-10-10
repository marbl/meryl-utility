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

#include "regex-v2.H"
#include "arrays.H"

//  Convert the input string to a list of tokens.
//  Each token represents either a letter to match (or a character class)
//  or an operation.
//    character classes ("[abcde]") are smashed to one token, as are
//    ranges ("{4,19}").  Ranges, '*', '?' and '+' are converted to
//    type Closure.

namespace merylutil::inline regex::inline v2 {


//  Returns 0..255 and advances nn over the character.
//
//  If the string is '\xDD', then the character returned is decoded from DD interpreted as hex digits.
//  Non-hex-digits in D are decoded to nonsense.
//
uint8
decodeCharClassChar(char const *str, uint64 &nn) {
  char  cl = 0;

  if      ((str[nn+0] == '\\') && (str[nn+1] == 'x') && isHexDigit(str[nn+2]) && isHexDigit(str[nn+2])) {
    nn++;  cl |= asciiHexToInteger(str[nn]) << 4;
    nn++;  cl |= asciiHexToInteger(str[nn]);
    nn++;
  }

  else {
    cl = str[nn++];
  }

  return cl;
}



//  Recognizes abbreviated character classes (\d etc) and non-abbreviated ('[:digit:]').
//  Returns fist letter in range and sets all bits to enable the full range.
//
//  If not a character class, returns str[nn].
//
bool
regExToken::matchCharacterClassToken(char const *str, uint64 &nn) {
  uint64  zz = 0;
  uint64  n1 = nn+1;

  _type = regExTokenType::rtCharClass;

  //fprintf(stderr, "matchCharacterClassToken()- nn=%lu '%s'\n", nn, str+nn);

  if       (str[nn] == '\\') {
    if      (str[n1] == 'a')  matchCharacterClassToken("[:alpha:]", zz);
    else if (str[n1] == 'w')  matchCharacterClassToken("[:alpha:]", zz), matchLetter('_');
    else if (str[n1] == 's')  matchCharacterClassToken("[:space:]", zz);
    else if (str[n1] == 'd')  matchCharacterClassToken("[:digit:]", zz);
    else if (str[n1] == 'l')  matchCharacterClassToken("[:lower:]", zz);
    else if (str[n1] == 'u')  matchCharacterClassToken("[:upper:]", zz);
    else if (str[n1] == 'p')  matchCharacterClassToken("[:print:]", zz);

    else if (str[n1] == 'W')  { regExToken d;  d.matchCharacterClassToken("[:alnum:]", zz); d.matchLetter('_'); d.invertMatch(); mergeMatches(d); }
    else if (str[n1] == 'D')  { regExToken d;  d.matchCharacterClassToken("[:digit:]", zz);                     d.invertMatch(); mergeMatches(d); }
    else if (str[n1] == 'S')  { regExToken d;  d.matchCharacterClassToken("[:space:]", zz);                     d.invertMatch(); mergeMatches(d); }

    else                      matchLetter(str[n1]);

    nn += 2;
  }

  else if ((str[nn] == '[') &&
           (str[n1] == ':')) {
    if      (strncmp(str+n1, "[:alnum:]",   9) == 0)  { matchCharacterClass("[A-Za-z0-9]", zz);              nn +=  9; }
    else if (strncmp(str+nn, "[:alpha:]",   9) == 0)  { matchCharacterClass("[A-Za-z]", zz);                 nn +=  9; }
    else if (strncmp(str+nn, "[:blank:]",   9) == 0)  { matchLetters(" \t");                                 nn +=  9; }
    else if (strncmp(str+nn, "[:cntrl:]",   9) == 0)  { matchCharacterClass("[\x00-\x1F\x7F]", zz);          nn +=  9; }
    else if (strncmp(str+nn, "[:digit:]",   9) == 0)  { matchCharacterClass("[0-9]", zz);                    nn +=  9; }
    else if (strncmp(str+nn, "[:graph:]",   9) == 0)  { matchCharacterClass("[\x21-\x7E]", zz);              nn +=  9; }
    else if (strncmp(str+nn, "[:lower:]",   9) == 0)  { matchCharacterClass("[a-z]", zz);                    nn +=  9; }
    else if (strncmp(str+nn, "[:print:]",   9) == 0)  { matchCharacterClass("[\x20-\x7E]", zz);              nn +=  9; }
    else if (strncmp(str+nn, "[:punct:]",   9) == 0)  { matchLetters("][!\"#$%&'()*+,./:;<=>?@\\^_`{|}~-");  nn +=  9; }
    else if (strncmp(str+nn, "[:space:]",   9) == 0)  { matchLetters(" \t\r\n\v\f");                         nn +=  9; }
    else if (strncmp(str+nn, "[:upper:]",   9) == 0)  { matchCharacterClass("[A-Z]", zz);                    nn +=  9; }
    else if (strncmp(str+nn, "[:xdigit:]", 10) == 0)  { matchCharacterClass("[0-9A-Za-z]", zz);              nn += 10; }
    else {
      return false;
    }
  }

  else {            //  Not starting with either '\' or '[:', so
    return false;   //  not something we decode.
  }

  return true;
}


          
//  Matches character classes specified either as an escaped class ('\d') or
//  as a range '[a-zGH]'.  Ranges can be specified using hex notation
//  ('\xDD') or with explicit letters: [\x00- ] would match everything before
//  (and including) a space.
//
//  A single '^' at the start of the class will invert the sense.
//
//  Control characters ARE NOT 
void
regExToken::matchCharacterClass(char const *str, uint64 &nn) {
  uint64 iv = 0;   //  If non-zero, invert the class at the end.
  uint8  c1 = 0;   //  The begin character in the range.
  uint8  c2 = 0;   //  The ending character in the range (possible the same as c1).

  _type = regExTokenType::rtCharClass;

  assert(str[nn] == '[');
  //fprintf(stderr, "matchCharacterClass()- nn=%lu '%s' (on ENTRY)\n", nn, str+nn);

  if (str[++nn] == '^')   //  Skip over the opening '[' then test for the invert symbol;
    iv = ++nn;            //  if found, set the flag and move past the invert symbol.

  while ((str[nn] != 0) && (str[nn] != ']')) {
    //fprintf(stderr, "matchCharacterClass()- nn=%lu '%s' (in LOOP)\n", nn, str+nn);

    if (matchCharacterClassToken(str, nn) == true)
      continue;

    c1 = decodeCharClassChar(str, nn);      //  Decode the first letter.
    c2 = c1;                                //  Make the range a single letter.

    if ((str[nn] != 0) && (str[nn] == '-')) {
      nn += 1;                              //  Skip the dash.
      c2 = decodeCharClassChar(str, nn);   //  Decode the second letter.
    }

    matchRange(c1, c2);
  }

  assert(str[nn] != 0);   //  We failed if we're at the end of the string.

  if (iv)
    invertMatch();
}






//  Decodes a group begin "({group}", "({capture}", "({prefix}" (and abbreviations)
//  ({group,prefix}
//  ({capture,prefix}
//  {{capture}
//  {{c}
//
//  ((:group:)
//
void
regExToken::makeGroupBegin(bool pfx, bool cap,
                           uint64 &grpIdent,
                           uint64 &capIdent) {
  _type     = regExTokenType::rtGroupBegin;

  _pfx      =  pfx;
  _cap      =  cap;
  _capIdent = (cap) ? ++capIdent : capIdent;
  _grpIdent =         ++grpIdent;
}

void
regExToken::makeGroupBegin(char const *str, uint64 ss, uint64 &nn,
                           uint64 &grpIdent,
                           uint64 &capIdent) {
  bool    pfx = false;
  bool    cap = false;
  uint64  err = 0;

  if ((str != nullptr) &&     //  If a string supplied,
      (str[nn+1] == '{')) {   //  Skip over the opening '(' then test for the modifier symbol;
    nn += 2;                  //  if found, skip both '(' and '{' and decode the modifiers.

    while ((str[nn] != 0) && (str[nn] != '}')) {
      if      (strncmp(str+nn, "group", 5) == 0)    { cap = false;  nn += 5;  }
      else if (strncmp(str+nn, "capture", 7) == 0)  { cap = true;   nn += 7;  }
      else if (strncmp(str+nn, "prefix", 6) == 0)   { pfx = true;   nn += 6;  }
      else if (str[nn] == 'g')                      { cap = false;  nn += 1;  }
      else if (str[nn] == 'c')                      { cap = true;   nn += 1;  }
      else if (str[nn] == 'p')                      { pfx = true;   nn += 1;  }
      else if (str[nn] == ',')                      {               nn += 1;  }
      else
        err = ++nn;
    }
  }

  if (err > 0)
    fprintf(stderr, "ERROR: expecting 'group', 'capture', 'prefix' in '%s'.\n", str+ss);

  makeGroupBegin(pfx, cap, grpIdent, capIdent);
}


void
regExToken::makeGroupEnd(uint64 grpIdent, uint64 capIdent) {
  _type     = regExTokenType::rtGroupEnd;
  _grpIdent = grpIdent;
  _capIdent = capIdent;
}







struct groupState {
  bool    pfx   = false;
  bool    cap   = false;
  uint64  depth = 0;
  uint64  cid   = 0;
  uint64  gid   = 0;
};



bool
regEx::parse(char const *str) {
  uint64                        tokIdent = uint64max;
  uint64                        grpIdent = uint64max;
  uint64                        capIdent = uint64max;
  regExToken                    toka     = { ._id=++tokIdent };

  merylutil::stack<groupState>  groupStates;   //  Stack of group information
  merylutil::stack<groupState>  prefxStates;   //  Stack of group information


  if (vParse) {
    fprintf(stderr, "\n");
    fprintf(stderr, "PARSING\n");
    fprintf(stderr, "\n");
  }

  resizeArray(tl, 0, tlMax, 1024);   //  Allocate an initial 1024 nodes.
  tlLen = 0;                         //  Recycle any existing token list.

  //  Make an initial capture group to get the entirety of the match.

  toka.makeGroupBegin(false, true, grpIdent, capIdent);

  tl[tlLen++] = toka;

  groupStates.push(groupState{ .pfx=toka._pfx, .cap=toka._cap, .depth=0, .cid=toka._capIdent, .gid=toka._grpIdent });



                            
  for (uint64 ss=0, nn=0; str[ss]; ss = nn) {    //  ss is the (constant) start of this token
    toka = { ._id=++tokIdent };                  //  nn is the (computed) start of the next token

    //  Handle escaped letters and character classes.
    //   /xHH   - matches character represented by hex digits HH
    //   /x     - matches any letter x
    //   .      - matches any character
    //   [ab-c] - match letters or ranges of letters
    //             - '^' at the start inverts the sense
    //             - ']' and '-' must be escaped

    if      (str[ss] == '\\')   toka.matchCharacterClassToken(str, nn), nn--;   //  Eats 2, but needs to
    else if (str[ss] == '[')    toka.matchCharacterClass(str, nn);              //    eat only one in this
    else if (str[ss] == '.')    toka.matchAllSymbols();                         //    loop.

    //  Grouping and capturing.
    //   (..)  - group the enclosed pattern into a single entity (see .H)

    else if (str[ss] == '(')  toka.makeGroupBegin(str, ss, nn, grpIdent, capIdent);
    else if (str[ss] == ')')  toka.makeGroupEnd(groupStates.top().gid, groupStates.top().cid);
#warning mismatched parens will crash the above

    //  Handle closures and alternation.
    //   *     - zero or more
    //   ?     - zero or one
    //   +     - one or more
    //   {a,b} - at least a, at most b; a=0, b=max if omitted

    else if (str[ss] == '*')  toka.makeClosure(0, uint64max);
    else if (str[ss] == '?')  toka.makeClosure(0, 1);
    else if (str[ss] == '+')  toka.makeClosure(1, uint64max);
    else if (str[ss] == '{')  toka.makeClosure(str, ss, nn);

    //  Alternation.  Concatenation is handled at the end.

    else if (str[ss] == '|')  toka.makeAlternation();

    //  Positional specifiers.
    //   ^      - start of the string; must be first letter
    //   $      - end of the string;   must be last letter

    else if (str[ss] == '^')  toka.matchLineStart();
    else if (str[ss] == '$')  toka.matchLineEnd();

    //  Anything else is a character.

    else                      toka.matchSymbol(str[ss]);

    //  Hooray!  Parsing of the token is finished.

    //  Ensure space exists for any new tokens we add:
    //    Possibly one extra for a prefix match.
    //    Always one for the symbol we just parsed.
    //    Up to 2 * prefixDepth if we're at the end of a prefix group.
    //    Possible one concatenation operator,
    //    
    while (tlLen + 1 + 1 + 2 * groupStates.top().depth + 1 >= tlMax)
      resizeArray(tl, tlLen, tlMax, tlMax + 1024);

    //  If we're in a prefix group, append a non-capture group begin before
    //  every symbol except the first:
    //    ({p}1234) -> (1(2(3(4?)?0?)?)?)
    //                   ^ ^ ^ - appended group begin
    //  and also count how many of these we have.

    if ((groupStates.top().pfx == true) &&                //  The last one is a prefix group
        ((toka._type == regExTokenType::rtCharClass) ||   //  The current token is a matching operator,
         (toka._type == regExTokenType::rtGroupBegin)) && //   or a group begin
        (groupStates.top().depth++ > 0)) {                //  And not the first   (Also count how many)
      tl[tlLen++] = { ._id=++tokIdent, ._type=regExTokenType::rtGroupBegin, ._grpIdent=++grpIdent };
      prefxStates.push(groupState{ .gid=grpIdent });
    }

    if ((groupStates.top().pfx == true) &&                //  Blow up on alternation in prefix groups,
        (toka._type == regExTokenType::rtAlternation)) {  //  because it makes no sense.
      fprintf(stderr, "Alteration not supported in prefix groups.\n");
      assert(0);
    }

    //  If this is a new group, remember group state.

    if (toka._type == regExTokenType::rtGroupBegin)
      groupStates.push(groupState{ .pfx=toka._pfx, .cap=toka._cap, .depth=0, .cid=toka._capIdent, .gid=toka._grpIdent });

    //  If this is a match operator, assign the match
    //  to the current capture group.

    if (toka._type == regExTokenType::rtCharClass)
      toka._capIdent = groupStates.top().cid;

    //  If we've just seen a group-end symbol, pop off the group-info for this group.
    //  If this is closing a prefix-group, terminate all the interal groups we made.

    if (toka._type == regExTokenType::rtGroupEnd) {
      groupState gs = groupStates.pop();

      if (gs.pfx == true) {
        while (gs.depth-- > 1) {
          groupState ps = prefxStates.pop();

          tl[tlLen++] = { ._id=++tokIdent, ._type=regExTokenType::rtGroupEnd, ._grpIdent=ps.gid };
          tl[tlLen++] = { ._id=++tokIdent, ._type=regExTokenType::rtClosure,  ._min=0, ._max=1 };
        }
      }
    }

    //  Append whatever token we made.

    tl[tlLen++] = toka;


    nn++;  //  Move to the next input character.

    //  Append a concatentaion operator if there is a next symbol and:
    //   - this was NOT a start group or alternation operator
    //        SYM ( concat     - makes no sense
    //        SYM | concat     - makes no sense
    //
    //   - the next symbol is NOT a closure operator (*, {, ?, +)
    //        SYM concat *     - makes no sense
    //
    //   - the next symbol is NOT an alternate operator or a group end
    //        SYM concat |     - makes no sense
    //        SYM concat )     - makes no sense
    //

    if ((str[nn] != 0) &&
        (str[ss] != '(') && (str[ss] != '|') &&
        (str[nn] != '*') && (str[nn] != '{') && (str[nn] != '?') && (str[nn] != '+') &&
        (str[nn] != '|') && (str[nn] != ')'))
      tl[tlLen++] = { ._id=++tokIdent, ._type=regExTokenType::rtConcat };

    assert(tlLen <= tlMax);
  }

  //  Close the overall capture group.
  tl[tlLen++] = { ._id=++tokIdent, ._type=regExTokenType::rtGroupEnd, ._capIdent=0, ._grpIdent=0 };

  assert(groupStates.top().gid == 0);
  assert(groupStates.top().cid == 0);

  capsLen = capIdent + 1;

  if (vParse) {
    for (uint64 ii=0; ii<tlLen; ii++)
      fprintf(stderr, "tl[%03lu] -- %s\n", ii, tl[ii].display());
    fprintf(stderr, " -- %lu capture groups\n", capsLen);
  }


  return true;
}

}  //  merylutil::regex::v2
