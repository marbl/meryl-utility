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
