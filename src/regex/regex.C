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

#include "types.H"
#include "files.H"
#include "system.H"
#include "math.H"

//  Precedence from high to low
enum class regexTokenType {
  rtGroupBegin,
  rtGroupEnd,
  rtAlternation,
  rtClosure,
  rtConcat,
  rtLineStart,
  rtLineEnd,
  rtCharClass,
  rtNone,
};

char const *toString(regexTokenType t) {
  switch (t) {
    case regexTokenType::rtGroupBegin:   return "GroupBegin";   break;
    case regexTokenType::rtGroupEnd:     return "GroupEnd";     break;
    case regexTokenType::rtAlternation:  return "Alternation";  break;
    case regexTokenType::rtClosure:      return "Closure";      break;
    case regexTokenType::rtConcat:       return "Concat";       break;
    case regexTokenType::rtLineStart:    return "LineStart";    break;
    case regexTokenType::rtLineEnd:      return "LineEnd";      break;
    case regexTokenType::rtCharClass:    return "CharClass";    break;
    case regexTokenType::rtNone:         return "None";         break;
    default:                             return "UNKNOWN";      break;
  }
}



class regexToken {
public:
  char const *name(void)     { return toString(_type); }

  void   setMatch(uint32 c)  {             (_syms[c>>6] |= (uint64one << (c & 0x3f)));  }
  bool   isMatch(uint32 c)   { return 0 != (_syms[c>>6] &  (uint64one << (c & 0x3f)));  }

public:
  void   matchLetter(char c) { _type    = regexTokenType::rtCharClass;  setMatch(c);                                             }
  void   matchNone(void)     { _type    = regexTokenType::rtCharClass;  for (uint32 ii=0; ii<4; ii++)  _syms[ii] = uint64zero;   }
  void   matchAll(void)      { _type    = regexTokenType::rtCharClass;  for (uint32 ii=0; ii<4; ii++)  _syms[ii] = uint64max;    }
  void   invertMatch(void)   {                                          for (uint32 ii=0; ii<4; ii++)  _syms[ii] = ~_syms[ii];   }

  char const *displayClass(char *str) {
    return nullptr;
  }

private:
  char    decodeCharacter(char const *str, uint32 &ss);

public:
  void    matchSymbol(char sym)                              { _type = regexTokenType::rtCharClass;  matchLetter(sym);                      }
  void    matchCharacter(char const *str, uint32 &ss)        { _type = regexTokenType::rtCharClass;  matchLetter(decodeCharacter(str, ss)); }
  void    matchCharacterClass(char const *str, uint32 &ss);

  void    makeClosure(char const *str, uint32 &ss);
  void    makeClosure(uint32 min, uint32 max) { _type = regexTokenType::rtClosure; _min=min; _max=max; };
  void    makeAlternation(void)               { _type = regexTokenType::rtAlternation; };
  void    makeConcatenation(void)             { _type = regexTokenType::rtConcat; };

  void    makeGroupBegin(void)  { _type = regexTokenType::rtGroupBegin; };
  void    makeGroupEnd(void)    { _type = regexTokenType::rtGroupEnd;   };

  void    matchLineStart(void)  { _type = regexTokenType::rtLineStart;  };
  void    matchLineEnd(void)    { _type = regexTokenType::rtLineEnd;    };

public:
  regexTokenType   _type    = regexTokenType::rtNone;
  uint64           _syms[4] = { 0, 0, 0, 0 };
  char             _sym     = 0;

  uint32           _id      = 0;
  uint32           _min     = 0;
  uint32           _max     = uint32max;
};



class regex {
public:
  regex(char const *str);
  ~regex();

  bool   stringToTokens(void);
  bool   tokensToReversePolish(void);
};




//  Decodes a possibly escaped character.
//    \xHH, matches hexadecimal character HH
//    \x    matches any letter x
//    x     matches any letter x
//
//  DOES NOT HANDLE ESCAPE PATTERNS
//
char
regexToken::decodeCharacter(char const *str, uint32 &ss) {
  char  cl = 0;

  if      ((str[ss+0] != 0) && (str[ss+0] == '\\') &&
           (str[ss+1] != 0) && (str[ss+1] == 'x') &&
           (str[ss+2] != 0) && isHexDigit(str[ss+2]) &&
           (str[ss+3] != 0) && isHexDigit(str[ss+3])) {
    cl = asciiHexToInteger(str[ss+2]) * 16 + asciiHexToInteger(str[ss+3]);
    ss += 4;
  }
  else if ((str[ss+0] != 0) && (str[ss+0] == '\\') &&
           (str[ss+1] != 0)) {
    cl = str[ss+1];
    ss += 2;
  }
  else {
    cl = str[ss];
    ss += 1;
  }

  return cl;
}


//  WARNING!  [a-] is invalid!  But [a-]] is valid.
//
void
regexToken::matchCharacterClass(char const *str, uint32 &ss) {
  uint32 iv = 0;   //  If non-zero, invert the class at the end.
  uint32 c1 = 0;   //  The begin character in the range.
  uint32 c2 = 0;   //  The ending character in the range (possible the same as c1).

  _type = regexTokenType::rtCharClass;

  if (str[++ss] == '^')   //  Skip over the opening '[' then test for the invert symbol;
    iv = ++ss;            //  if found, set the flag and move past the invert symbol.

  while ((str[ss] != 0) && (str[ss] != ']')) {
    c1 = decodeCharacter(str, ss);    //  Decode the first letter.
    c2 = c1;                          //  Make the range a single letter.

    if ((str[ss] != 0) && (str[ss] == '-')) {
      ss += 1;                        //  Skip the dash.
      c2 = decodeCharacter(str, ss);  //  Decode the second letter.
    }

    for (uint32 ii=c1; ii<=c2; ii++)  //  Set all letters in the
      matchLetter(ii);                //  range to true.
  }

  assert(str[ss] != 0);   //  We failed if we're at the end of the string.

  if (iv)
    invertMatch();
}




//  Decodes a closure range "{a,b}"
void
regexToken::makeClosure(char const *str, uint32 &ss) {
  char const *dec = str + ss;

  _type = regexTokenType::rtClosure;

  _min=0;          if (*dec != ',') dec = strtonumber(dec+1, _min);   assert(*dec == ',');   dec++;
  _max=uint32max;  if (*dec != '}') dec = strtonumber(dec,   _max);   assert(*dec == '}');

  ss = dec - str;
}





//  Convert the input string to a list of tokens.
//  Each token represents either a letter to match (or a character class)
//  or an operation.
//    character classes ("[abcde]") are smashed to one token, as are
//    ranges ("{4,19}").  Ranges, '*', '?' and '+' are converted to
//    type Closure.
//
bool
parse1(char const *&str,
       regexToken *tl, uint32 &tlLen, uint32 &tlMax) {
  uint32 id=0;

  for (uint32 ss=0; str[ss]; ss++) {
    regexToken  toka = { ._id=id++ };

    //  Handle escaped letters and character classes.
    //   /xHH   - matches character represented by hex digits HH
    //   /x     - matches any letter x
    //   .      - matches any character
    //   [ab-c] - match letters or ranges of letters
    //             - '^' at the start inverts the sense
    //             - ']' and '-' must be escaped

    if      (str[ss] == '\\') toka.matchCharacter(str, ss);
    else if (str[ss] == '[')  toka.matchCharacterClass(str, ss);
    else if (str[ss] == '.')  toka.matchAll();

    //  Handle closures and alternation.
    //   *     - zero or more
    //   ?     - zero or one
    //   +     - one or more
    //   {a,b} - at least a, at most b; a=0, b=max if omitted

    else if (str[ss] == '*')  toka.makeClosure(0, uint32max);
    else if (str[ss] == '?')  toka.makeClosure(0, 1);
    else if (str[ss] == '+')  toka.makeClosure(1, uint32max);
    else if (str[ss] == '{')  toka.makeClosure(str, ss);

    //  Alternation.  Concatenation is handled at the end.

    else if (str[ss] == '|')  toka.makeAlternation();  // = { ._type = regexTokenType::rtAlternation };

    //  Grouping and positional specifiers.
    //   ()     - group the enclosed pattern into a single entity
    //   ^      - start of the string; must be first letter
    //   $      - end of the string;   must be last letter

    else if (str[ss] == '(')  toka.makeGroupBegin();// = { ._type = regexTokenType::rtGroupBegin };
    else if (str[ss] == ')')  toka.makeGroupEnd();// = { ._type = regexTokenType::rtGroupEnd   };
    else if (str[ss] == '^')  toka.matchLineStart();// = { ._type = regexTokenType::rtLineStart };
    else if (str[ss] == '$')  toka.matchLineEnd();// = { ._type = regexTokenType::rtLineEnd };

    //  Anything else is a character.

    else                      toka.matchSymbol(str[ss]);

    //  Append whatever token we made and then append a concatentaion operator if:
    //    this is NOT a grouping sentinel, or
    //    the next symbol is NOT something that operates on the current symbol
    //    there IS a next symbol

    tl[tlLen++] = toka;

    if ((str[ss+0] != '(') && (str[ss+0] != ')') &&
        (str[ss+1] != '*') && (str[ss+1] != '{') && (str[ss+1] != '?') && (str[ss+1] != '+') && (str[ss+1] != '|') && (str[ss+1] != ')') &&
        (str[ss+1] !=  0))
      tl[tlLen++] = { ._type = regexTokenType::rtConcat, ._id=id++ };
  }

  return true;
}



#if 0
void
displayTokens(void) {
  for (uint32 ii=0; ii<tlLen; ii++) {
    regexToken T = tl[ii];

    switch (T._type) {
      case regexTokenType::rtGroupBegin:
      case regexTokenType::rtGroupEnd:
        fprintf(stdout, "infix %2u %-10s\n", T._id, T.name());
        break;
      case regexTokenType::rtAlternation:
        fprintf(stdout, "infix %2u %-10s\n", T._id, T.name());
         break;
      case regexTokenType::rtClosure:
        fprintf(stdout, "infix %2u %-10s min:%u max:%u\n", T._id, T.name(), T._min, T._max);
        break;
      case regexTokenType::rtConcat:
        fprintf(stdout, "infix %2u %-10s\n", T._id, T.name());
        break;
      case regexTokenType::rtLineStart:
      case regexTokenType::rtLineEnd:
        fprintf(stdout, "infix %2u %-10s\n", T._id, T.name());
        break;
      case regexTokenType::rtCharClass:
        fprintf(stdout, "infix %2u %-10s ", T._id, T.name());
        if (T._sym != 0) {
          fprintf(stdout, "'%c'\n", T._sym);
        }
        else {
          fprintf(stdout, "[");

          for (uint32 cc=0; cc<256; cc++) {
            if (T.isInside(cc) == false)
              continue;

            uint32 ee=cc + 1;
            while ((ee < 256) && (T.isInside(ee) == true))
              ee++;
            ee--;

            if (cc == ee) {
              if (isVisible(cc))
                fprintf(stdout, "%c", cc);
              else
                fprintf(stdout, "\\x%02x", cc);
            }
            else if (cc == ee-1) {
              if (isVisible(cc))
                fprintf(stdout, "%c", cc);
              else
                fprintf(stdout, "\\x%02x", cc);

              if (isVisible(ee))
                fprintf(stdout, "%c", ee);
              else
                fprintf(stdout, "\\x%02x", ee);
            }
            else {
              if (isVisible(cc))
                fprintf(stdout, "%c", cc);
              else
                fprintf(stdout, "\\x%02x", cc);

              fprintf(stdout, "-");

              if (isVisible(ee))
                fprintf(stdout, "%c", ee);
              else
                fprintf(stdout, "\\x%02x", ee);
            }

            cc=ee;
          }

          fprintf(stdout, "]\n");
        }
        break;
      case regexTokenType::rtNone:
        fprintf(stdout, "infix %2u %-10s\n", T._id, T.name());
        break;
      default:
        fprintf(stdout, "infix %2u %-10s\n", T._id, T.name());
        break;
    }
  }
}
#endif





//  Takes the tokenized and concat-inserted input
//  and converts it to reverse-polish.
bool
parse2(regexToken *tl, uint32 &tlLen, uint32 &tlMax,
       regexToken *st, uint32 &stLen, uint32 &stMax) {

  uint32 olLen = 0;

    //  If an operation, pop off higher precedence operations.
  //
    //  If the start of a group, push on the operation.
  //
    //  If the end of a group, pop the stack until we get
    //  to the first group open, then chuck the open.
  //
    //  Otherwise, just move the token to the output.
    //    rtNone, rtLineStart, rtLineEnd, rtCharClatt.
  //
  for (uint32 tt=0; tt<tlLen; tt++) {
    //fprintf(stdout, "---\n");
    //fprintf(stdout, "sym %2u %-10s %c %2d\n", tt, tl[tt].name(), tl[tt]._sym, tl[tt]._id);

    if ((tl[tt]._type == regexTokenType::rtAlternation) ||
        (tl[tt]._type == regexTokenType::rtConcat) ||
        (tl[tt]._type == regexTokenType::rtClosure)) {
      while ((stLen > 0) && (st[stLen-1]._type != regexTokenType::rtGroupBegin) && (st[stLen-1]._type <= tl[tt]._type)) {
        tl[olLen++] = st[--stLen];
        //fprintf(stdout, "pop %2u %-10s %c %2d\n", olLen-1, tl[olLen-1].name(), tl[olLen-1]._sym, tl[olLen-1]._id);
      }
      //fprintf(stdout, "push   %-10s %c %2d\n", tl[tt].name(), tl[tt]._sym, tl[tt]._id);
      st[stLen++] = tl[tt];
    }

    else if (tl[tt]._type == regexTokenType::rtGroupBegin) {
      //fprintf(stdout, "push   %-10s %c %2d\n", tl[tt].name(), tl[tt]._sym, tl[tt]._id);
      st[stLen++] = tl[tt];
    }

    else if (tl[tt]._type == regexTokenType::rtGroupEnd) {
      while ((stLen > 0) && (st[stLen-1]._type != regexTokenType::rtGroupBegin)) {
        tl[olLen++] = st[--stLen];
        //fprintf(stdout, "end %2u %-10s %c %2d\n", olLen-1, tl[olLen-1].name(), tl[olLen-1]._sym, tl[olLen-1]._id);
      }
      assert(stLen > 0);
      --stLen;
    }

    else {
      if (olLen != tt)
        tl[olLen] = tl[tt];
      olLen++;
      //fprintf(stdout, "    %2u %-10s %c %2d\n", olLen-1, tl[olLen-1].name(), tl[olLen-1]._sym, tl[olLen-1]._id);
    }

  }

  //  Clear the stack.
  while (stLen > 0)
    tl[olLen++] = st[--stLen];

  tlLen = olLen;

  return true;
}



int
main(int argc, char const **argv) {


  if (argc == 1) {
    fprintf(stderr, "usage: %s <regex-string>\n", argv[0]);
    exit(1);
  }

  fprintf(stdout, "%s\n", argv[1]);

  uint32       tlMax = strlen(argv[1]) * 2;
  uint32       tlLen = 0;
  regexToken  *tl    = new regexToken [tlMax];

  uint32       stMax = strlen(argv[1]);
  uint32       stLen = 0;
  regexToken  *st    = new regexToken [stMax];

  fprintf(stdout, "PARSE 1:\n");
  parse1(argv[1], tl, tlLen, tlMax);


  fprintf(stdout, "PARSE 2:\n");
  parse2(         tl, tlLen, tlMax, st, stLen, stMax);

#if 1
  for (uint32 ii=0; ii<stLen; ii++)
    fprintf(stdout, "st %2u %-10s %c %2d\n", ii, st[ii].name(), st[ii]._sym, st[ii]._id);
  for (uint32 ii=0; ii<tlLen; ii++)
    fprintf(stdout, "tl %2u %-10s %c %2d\n", ii, tl[ii].name(), tl[ii]._sym, tl[ii]._id);
#endif

  delete [] st;
  delete [] tl;

  return 0;
}




















