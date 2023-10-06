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
      app += sprintf(app, "group bgn  id:%2u grp:%03lu%s%s", _id, _grpIdent, _cap ? " CAP" : "", _pfx ? " PFX" : "");
      break;
    case regExTokenType::rtGroupEnd:
      app += sprintf(app, "group end  id:%2u grp:%03lu", _id, _grpIdent);
      break;
    case regExTokenType::rtAlternation:
      app += sprintf(app, "alternat   id:%2u", _id);
      break;
    case regExTokenType::rtClosure:
      app += sprintf(app, "closure    id:%2u         min:%u max:%u", _id, _min, _max);
      break;
    case regExTokenType::rtConcat:
      app += sprintf(app, "concat     id:%2u", _id);
      break;
    case regExTokenType::rtLineStart:
      app += sprintf(app, "line-start id:%2u", _id);
      break;
    case regExTokenType::rtLineEnd:
      app += sprintf(app, "line-end   id:%2u", _id);
      break;
    case regExTokenType::rtCharClass:
      app += sprintf(app, "charact    id:%2u grp:%03lu ", _id, _grpIdent);

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

        app += sprintf(app, "]");
      }
      break;
    case regExTokenType::rtNone:
      app += sprintf(app, "NONE       id:%2u", _id);
      break;
    default:
      app += sprintf(app, "DEFAULT    id:%2u", _id);
      break;
  }

  return out;
}



//  Decodes a closure range:
//    #1 - {a,b}
//    #2 - {a}
//    #3 - {a,}
//    #4 - {,b}
//
//  It's pretty tricky code.
//    1)  Skip any open brace.  We'll either be on 'a' or a comma now.
//    2)  If on 'a' (#1, #2, #3), decode it into _min.
//        We'll be on the comma or closing brace now.
//    3)  If on the closing brace (#2), set _max to _min.  We're done.
//    4)  If on the comma (#1, #3, #4), skip over it.
//    5)  If on 'b' (#1 and #4), decode it into _max.
//        We'll be on the closing brace now.
//        For case #3, _max will remain at uint64max.
//    6)  Update string pointer and ensure we're at the closing brace.
void
regExToken::makeClosure(char const *str, uint64 ss, uint64 &nn) {
  char const *dec = str + nn;

  _type = regExTokenType::rtClosure;
  _min  = 0;
  _max  = uint64max;

  //fprintf(stderr, "makeClosure()-- '%s'\n", str+nn);

  if (dec[0] == '{')  dec++;
  if (dec[0] != ',')  dec = strtonumber(dec, _min);
  if (dec[0] == '}')  _max = _min;
  if (dec[0] == ',')  dec++;
  if (dec[0] != '}')  dec = strtonumber(dec, _max);
  if (dec[0] != '}')  fprintf(stderr, "Failed to decode closure range '%s'\n", str + nn);

  assert(dec[0] == '}');

  //fprintf(stderr, "makeClosure()-- '%s' -> %lu %lu\n", str+nn, _min, _max);

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
