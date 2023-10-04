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

namespace merylutil::inline regex::inline v2 {


char const *
regExToken::display(char *str) {
  static
  char  dis[1024];
  char *out = dis;
  char *app = dis;

  if (str)
    out = app = str;

  switch (_type) {
    case regExTokenType::rtGroupBegin:
      app += sprintf(app, "group bgn  %2u %-10s capture:%d grpIdent:%u prefix:%d", _id, name(), _cap, _grpIdent, _pfx);
      break;
    case regExTokenType::rtGroupEnd:
      app += sprintf(app, "group end  %2u %-10s            grpIdent:%u", _id, name(), _grpIdent);
      break;
    case regExTokenType::rtAlternation:
      app += sprintf(app, "alternat   %2u %-10s", _id, name());
      break;
    case regExTokenType::rtClosure:
      app += sprintf(app, "closure    %2u %-10s min:%u max:%u", _id, name(), _min, _max);
      break;
    case regExTokenType::rtConcat:
      app += sprintf(app, "concat     %2u %-10s", _id, name());
      break;
    case regExTokenType::rtLineStart:
      app += sprintf(app, "line-start %2u %-10s", _id, name());
      break;
    case regExTokenType::rtLineEnd:
      app += sprintf(app, "line-end   %2u %-10s", _id, name());
      break;
    case regExTokenType::rtCharClass:
      app += sprintf(app, "charact    %2u %-10s ", _id, name());
      if (_sym != 0) {
        app += sprintf(app, "'%c'", _sym);
      }
      else {
        app += sprintf(app, "[");

        for (uint32 cc=0; cc<256; cc++) {   //  uint8 unsafe!
          if (isMatch(cc) == false)
            continue;

          uint32 ee=cc + 1;  //  uint8 unsafe
          while ((ee < 256) && (isMatch(ee) == true))
            ee++;
          ee--;

          if (cc == ee) {
            if (isVisible(cc))
              app += sprintf(app, "%c", cc);
            else
              app += sprintf(app, "\\x%02x", cc);
          }
          else if (cc == ee-1) {
            if (isVisible(cc))
              app += sprintf(app, "%c", cc);
            else
              app += sprintf(app, "\\x%02x", cc);

            if (isVisible(ee))
              app += sprintf(app, "%c", ee);
            else
              app += sprintf(app, "\\x%02x", ee);
          }
          else {
            if (isVisible(cc))
              app += sprintf(app, "%c", cc);
            else
              app += sprintf(app, "\\x%02x", cc);

            app += sprintf(app, "-");

            if (isVisible(ee))
              app += sprintf(app, "%c", ee);
            else
              app += sprintf(app, "\\x%02x", ee);
          }

          cc=ee;
        }

        app += sprintf(app, "]  grpIdent %u", _grpIdent);
      }
      break;
    case regExTokenType::rtNone:
      app += sprintf(app, "NONE       %2u %-10s", _id, name());
      break;
    default:
      app += sprintf(app, "DEFAULT    %2u %-10s", _id, name());
      break;
  }

  return out;
}







//  Decodes a possibly escaped character.
//    \xHH, matches hexadecimal character HH
//    \x    matches any letter x
//    x     matches any letter x
//
//  DOES NOT HANDLE ESCAPE PATTERNS
//
char
regExToken::decodeCharacter(char const *str, uint64 &nn) {
  char  cl = 0;

  if      ((str[nn+0] != 0) && (str[nn+0] == '\\') &&
           (str[nn+1] != 0) && (str[nn+1] == 'x') &&
           (str[nn+2] != 0) && isHexDigit(str[nn+2]) &&
           (str[nn+3] != 0) && isHexDigit(str[nn+3])) {
    cl = asciiHexToInteger(str[nn+2]) * 16 + asciiHexToInteger(str[nn+3]);
    nn += 4;
  }
  else if ((str[nn+0] != 0) && (str[nn+0] == '\\') &&
           (str[nn+1] != 0)) {
    cl = str[nn+1];
    nn += 2;
  }
  else {
    cl = str[nn];
    nn += 1;
  }

  return cl;
}


//  WARNING!  [a-] is invalid!  But [a-]] is valid.
//
void
regExToken::matchCharacterClass(char const *str, uint64 &nn) {
  uint64 iv = 0;   //  If non-zero, invert the class at the end.
  uint8  c1 = 0;   //  The begin character in the range.
  uint8  c2 = 0;   //  The ending character in the range (possible the same as c1).

  _type = regExTokenType::rtCharClass;

  if (str[++nn] == '^')   //  Skip over the opening '[' then test for the invert symbol;
    iv = ++nn;            //  if found, set the flag and move past the invert symbol.

  while ((str[nn] != 0) && (str[nn] != ']')) {
    c1 = decodeCharacter(str, nn);     //  Decode the first letter.
    c2 = c1;                           //  Make the range a single letter.

    if ((str[nn] != 0) && (str[nn] == '-')) {
      nn += 1;                         //  Skip the dash.
      c2 = decodeCharacter(str, nn);   //  Decode the second letter.
    }

    for (uint32 ii=c1; ii<=c2; ii++)   //  Set all letters in the    uint8 unsafe!
      matchLetter(ii);                 //  range to true.
  }

  assert(str[nn] != 0);   //  We failed if we're at the end of the string.

  if (iv)
    invertMatch();
}



//  Decodes a closure range "{a,b}"
void
regExToken::makeClosure(char const *str, uint64 ss, uint64 &nn) {
  char const *dec = str + nn;

  _type = regExTokenType::rtClosure;

  _min=0;          if (*dec != ',') dec = strtonumber(dec+1, _min);   assert(*dec == ',');   dec++;
  _max=uint64max;  if (*dec != '}') dec = strtonumber(dec,   _max);   assert(*dec == '}');

  nn = dec - str;
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
regExToken::makeGroupBegin(char const *str, uint64 ss, uint64 &nn, uint64 grpIdent) {
  uint64  err = 0;

  _type     = regExTokenType::rtGroupBegin;
  _cap      = true;
  _grpIdent = grpIdent;
  _pfx      = false;

  if (str[nn+1] == '{') {   //  Skip over the opening '(' then test for the modifier symbol;
    nn += 2;                //  if found, skip both '(' and '{' and decode the modifiers.

    while ((str[nn] != 0) && (str[nn] != '}')) {
      if      (strncmp(str+nn, "group", 5) == 0)    { _cap = false;  nn += 5;  }
      else if (strncmp(str+nn, "capture", 7) == 0)  { _cap = true;   nn += 7;  }
      else if (strncmp(str+nn, "prefix", 6) == 0)   { _pfx = true;   nn += 6;  }
      else if (str[nn] == 'g')                      { _cap = false;  nn += 1;  }
      else if (str[nn] == 'c')                      { _cap = true;   nn += 1;  }
      else if (str[nn] == 'p')                      { _pfx = true;   nn += 1;  }
      else if (str[nn] == ',')                      {                nn += 1;  }
      else
        err = ++nn;
    }
  }

  if (err > 0) {
    fprintf(stderr, "ERROR: expecting 'group', 'capture', 'prefix' in '%s'.\n", str+ss);
  }
}


void
regExToken::makeGroupEnd(uint64 grpIdent) {
  _type     = regExTokenType::rtGroupEnd;
  _grpIdent = grpIdent;
}


}  //    merylutil::regex::v2
