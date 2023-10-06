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

#ifndef MERYLUTIL_REGEX_H
#define MERYLUTIL_REGEX_H

//
//  A simple regex parser.
//    BRE - Basic Regular Expressions (POSIX)
//    ERE - Extended Regular Expressions (POSIX)
//    SRE - Simple Regular Expressions (deprecated in favor of BRE)
//    PRE - Perl Regular Expressions
//
//  ERE adds ?, + and |, and removes the requirement that () and {} are escaped.
//  
//  ^      -- matches start of line
//  $       -- matches end of line (either end of string or newline)
//  .      -- matches any character (except newline)
//  []     -- matches any of a set of characters
//  [^]    -- matches any except a set of characters
//  \n     -- BRE only, matches a previously matched group
//  *      -- matches previous element zero or more times (append ? to match minimally)
//  {m,n}  -- matches previous element at least m and at most n times (append ? to match minimally)
//
//  ?      -- matches previous element zero or one times (append ? to match minimally)
//  +      -- matches previous element at lest once (append ? to match minimally)
//  |      -- matches either the expression before or the expression after
//
//  (..)   -- grouping and capturing
//  <..>   -- grouping only - (?:..) is the standard notation
//
//  NOTE:  Edge cases in nested captures exist.
//              (ab(c?)) - will return 'ab' and 'c' for captures, instead of 'abc' and 'c'.
//
//  Still need something for prefix match in a group.
//    ({g}pattern)  - disables capturing
//    ({c}pattern)  - explicitly enables capturing
//    ({p}pattern)  - captures any prefix of the pattern
//
//    ({prefix}example) --> (e(x(a(m(p(le?)?)?)?)?)?)
//
//  Character classes, used within bracket expressions:
//    [:alnum:]  - [A-Za-z0-9]
//    [:alpha:]  - [A-Za-z]
//    [:blank:]  - [ \t]            - space and tab
//    [:cntrl:]  - [\x00-\x1F\x7F]
//    [:digit:]  - [0-9]
//    [:graph:]  - [\x21-\x7E]      - visible characters
//    [:lower:]  - [a-z]
//    [:print:]  - [\x20-\x7E]
//    [:punct:]  - [][!"#$%&'()*+,./:;<=>?@\^_`{|}~-]
//    [:space:]  - [ \t\r\n\v\f]
//    [:upper:]  - [A-Z]
//    [:xdigit:] - [A-Fa-f0-9]
//

#include "types.H"

namespace merylutil::inline regex::inline v2 {

//  Precedence from high to low
enum class regExTokenType {
  rtGroupBegin,
  rtGroupEnd,
  rtClosure,
  rtConcat,
  rtAlternation,
  rtLineStart,
  rtLineEnd,
  rtCharClass,
  rtNone,
};



class regExToken {
public:
  char const *display(char *str=nullptr);

  void   setMatch(uint8 c)   {             (_syms[c>>6] |= (uint64one << (c & 0x3f)));  }
  bool   isMatch(uint8 c)    { return 0 != (_syms[c>>6] &  (uint64one << (c & 0x3f)));  }

public:
  void   matchLetter(uint8 c)                  {                                 setMatch(c);              }
  void   matchLetters(char const *s)           { for (uint16 ii=0; s[ii]; ii++)  setMatch(s[ii]);          }
  void   matchRange(uint8 a, uint8 b)          { for (uint16  c=a;  c<=b;  c++)  setMatch(c);              } 

  void   invertMatch(void)                     { for (uint8  ii=0; ii< 4; ii++)  _syms[ii] = ~_syms[ii];   }

  void   mergeMatches(regExToken const &d)     { for (uint8  ii=0; ii< 4; ii++)  _syms[ii] |= d._syms[ii];  }

private:
  bool    decodeCharClassToken(char const *str, uint64 &nn);

public:
  void    matchAllSymbols(void)                { _type = regExTokenType::rtCharClass;  matchRange(0x00, 0xff); }
  void    matchSymbol(char sym)                { _type = regExTokenType::rtCharClass;  matchLetter(sym);       }

  bool    matchCharacterClassToken(char const *str, uint64 &nn);
  void    matchCharacterClass     (char const *str, uint64 &nn);

  void    makeClosure(char const *str, uint64 ss, uint64 &nn);
  void    makeClosure(uint64 min, uint64 max) { _type = regExTokenType::rtClosure; _min=min; _max=max; };
  void    makeAlternation(void)               { _type = regExTokenType::rtAlternation;  };
  void    makeConcatenation(void)             { _type = regExTokenType::rtConcat;       };

  void    makeGroupBegin(char const *str, uint64 ss, uint64 &nn, uint64 grpIdent);
  void    makeGroupEnd  (                                        uint64 grpIdent);

  void    matchLineStart(void)                { _type = regExTokenType::rtLineStart;    };
  void    matchLineEnd(void)                  { _type = regExTokenType::rtLineEnd;      };

public:
  uint64           _id        = 0;
  regExTokenType   _type      = regExTokenType::rtNone;

  uint64           _syms[4]   = { 0, 0, 0, 0 };
  char             _sym       = 0;

  bool             _cap       = false;
  bool             _pfx       = false;

  uint64           _grpIdent  = 0;   //  group ident of any capture group, default to 'all'

  uint64           _min       = 0;
  uint64           _max       = uint64max;
};



class regExState {
public:
  regExState() {};
  ~regExState() {};

  void         addMatch(regExToken tok, regExState *to, bool verbose=false) {

    if (_match)
      fprintf(stderr, "ERROR: can't add second match rule to state id=%lu.\n", _id);
    if (_lambda[0])
      fprintf(stderr, "ERROR: can't add match rule to state id=%lu; has lambda rules.\n", _id);

    assert(_match == nullptr);
    assert(_lambda[0] == nullptr);

    if (verbose)
      fprintf(stderr, "  addMatch()   %-3u match  -> %u\n", _id, to->_id);

    _tok   = tok;
    _match = to;
  }
  void         addEpsilon(regExState *to, bool verbose=false) {

    if (_match)
      fprintf(stderr, "ERROR: can't add epsilon to state id=%lu; has existing _match rule.\n", _id);
    if (_lambda[1])
      fprintf(stderr, "ERROR: can't add third epsilon rule to state id=%lu.\n", _id);

    assert(_match     == nullptr);
    assert(_lambda[1] == nullptr);

    if      (_lambda[0] == nullptr)  _lambda[0] = to;
    else if (_lambda[1] == nullptr)  _lambda[1] = to;
    else                             assert(0);

    if (verbose)
      fprintf(stderr, "  addEpsilon() %-3u lambda -> %u\n", _id, to->_id);
  }

  uint64      _id         = 0;
  bool        _accepting  = false;

  regExToken  _tok;

  regExState  *_match     =   nullptr;
  regExState  *_lambda[2] = { nullptr, nullptr };
};



class regExExpr {
public:
  regExExpr(regExState *b=nullptr, regExState *e=nullptr) : _bgn(b), _end(e) {};
  ~regExExpr() {};

  regExExpr     duplicate(regExState *rs, uint64 &rsLen, bool verbose=false);

  regExState  *_bgn;
  regExState  *_end;
};



class regEx {
public:
  regEx()  {
  };
  ~regEx() {
    delete [] tl;
    delete [] rs;
    delete [] capstor;
    delete [] bgnP;
    delete [] endP;
    delete [] caps;
  };

public:
  void        enableVerbose(char which='a') {
    if ((which == 'a') || (which == 'p'))  vParse   = true;
    if ((which == 'a') || (which == 'c'))  vConvert = true;
    if ((which == 'a') || (which == 'b'))  vBuild   = true;
    if ((which == 'a') || (which == 'm'))  vMatch   = true;
  }
  void        disableVerbose(char which='a') {
    if ((which == 'a') || (which == 'p'))  vParse   = false;
    if ((which == 'a') || (which == 'c'))  vConvert = false;
    if ((which == 'a') || (which == 'b'))  vBuild   = false;
    if ((which == 'a') || (which == 'm'))  vMatch   = false;
  }

public:
  bool        construct(char const *pattern) {
    parse(pattern);
    convert();
    build();
    return true;
  }
  bool        match(char const *query);

  uint64      numCaptures(void)         { return capsLen;   }

  uint64      getCaptureBgn(uint64 cap) { return bgnP[cap]; }
  uint64      getCaptureEnd(uint64 cap) { return endP[cap]; }
  char const *getCapture   (uint64 cap) { return caps[cap]; }

private:
  regExExpr   concat   (regExState *rs, uint64 &rsLen, regExExpr a, regExExpr b);
  regExExpr   alternate(regExState *rs, uint64 &rsLen, regExExpr a, regExExpr b);
  regExExpr   closure  (regExState *rs, uint64 &rsLen, regExExpr a, uint64 min, uint64 max);
  regExExpr   symbol   (regExState *rs, uint64 &rsLen, regExToken tok);

  bool        followStates(regExState *in, regExState **sB, uint64 &sBlen);

  bool        parse(char const *pattern);
  bool        convert(void);
  bool        build(void);

private:
  bool         vParse   = false;
  bool         vConvert = false;
  bool         vBuild   = false;
  bool         vMatch   = false;

  uint64       tlMax = 0;         //  A list of tokens, the output of parse().
  uint64       tlLen = 0;
  regExToken  *tl    = nullptr;

  uint64       rsMax = 0;         //  A set of allocated NFA states.
  uint64       rsLen = 0;
  regExState  *rs    = nullptr;

  regExExpr    re;

  uint64       capstorLen = 0;
  uint64       capstorMax = 0;
  char        *capstor    = nullptr;

  uint64       capsMax    = 0;
  uint64       capsLen    = 0;
  uint64      *bgnP       = nullptr;
  uint64      *endP       = nullptr;
  char const **caps       = nullptr;
};


}  //  merylutil::regex::v2

#endif  //  MERYLUTIL_REGEX_H
