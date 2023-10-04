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

struct groupState {
  bool    pfx   = false;
  bool    cap   = false;
  uint64  depth = 0;
  uint64  ident = 0;
};


bool
regEx::parse(char const *str) {
  uint64                        tokIdent = 0;
  uint64                        grpIdent = uint64max;  //  Is pre-incremented to 0 on first use.

  merylutil::stack<groupState>  groupStates;   //  Stack of group information
  merylutil::stack<uint64>      capIdent;      //  Ident of the capture groups

  resizeArray(tl, 0, tlMax, 1024);   //  Allocate an initial 1024 nodes.
  tlLen = 0;                         //  Recycle any existing token list.

  //  Make an initial capture group to get the entirety of the match.
  //tl[tlLen++] = makeGroupBegin(true, ++grpIdent);
  tl[tlLen++] = { ._id=tokIdent++, ._type=regExTokenType::rtGroupBegin, ._cap=true, ._grpIdent=++grpIdent };

  for (uint64 ss=0, nn=0; str[ss]; ss = nn) {    //  ss is the (constant) start of this token
    regExToken  toka = { ._id=tokIdent++ };      //  nn is the (computed) start of the next token

    //  Handle escaped letters and character classes.
    //   /xHH   - matches character represented by hex digits HH
    //   /x     - matches any letter x
    //   .      - matches any character
    //   [ab-c] - match letters or ranges of letters
    //             - '^' at the start inverts the sense
    //             - ']' and '-' must be escaped

    if      (str[ss] == '\\') toka.matchCharacter(str, nn);
    else if (str[ss] == '[')  toka.matchCharacterClass(str, nn);
    else if (str[ss] == '.')  toka.matchAll();

    //  Grouping and capturing.
    //   (..)  - group the enclosed pattern into a single entity (see .H)

    else if (str[ss] == '(')  toka.makeGroupBegin(str, ss, nn, ++grpIdent);
    else if (str[ss] == ')')  toka.makeGroupEnd(groupStates.top().ident);
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
    while (tlLen + 1 + 1 + 2 * (groupStates.empty() ? 0 : groupStates.top().depth) + 1 >= tlMax)
      resizeArray(tl, tlLen, tlMax, tlMax + 1024);

    //  If a new group created, push on state for prefix creation.
    //  If the group is a capture group, remember the ident so we can set match states.

    if (toka._type == regExTokenType::rtGroupBegin) {
      groupStates.push(groupState{ .pfx = toka._pfx, .cap = toka._cap, .depth = 0, .ident = grpIdent });

      if (toka._cap == true) {
        capIdent.push(grpIdent);
        capsLen = std::max(capsLen, grpIdent+1);
      }
    }

    //  If we're in a prefix group, append a non-capture group begin before
    //  every symbol except the first:
    //    ({p}1234) -> (1(2(3(4?)?0?)?)?)
    //                   ^ ^ ^ - appended group begin
    //  and also count how many of these we have.

    if ((groupStates.empty() == false) &&               //  Groups exist?
        (groupStates.top().pfx == true) &&              //  The last one is a prefix group?
        (toka._type == regExTokenType::rtCharClass) &&  //  The current token is a matching operator?
        (groupStates.top().depth++ > 0))                //  And not the first?  (Also count how many)
      tl[tlLen++] = { ._id=tokIdent++, ._type=regExTokenType::rtGroupBegin, ._cap=false };

    //  If this is a match operator, and there is an active capture group, assign the match
    //  to the capture group.  The default group is '0', the overall match.

    if ((toka._type == regExTokenType::rtCharClass) && (capIdent.empty() == false))
      toka._grpIdent = capIdent.top();

    //  Append whatever token we made.

    tl[tlLen++] = toka;

    //  If we've just seen a group-end symbol, pop off the group-info for this group.
    //  If this is closing a prefix-group, terminate all the interal groups we made.

    if (toka._type == regExTokenType::rtGroupEnd) {
      groupState gs = groupStates.pop();

      if (gs.pfx == true) {
        while (gs.depth-- > 1) {
          tl[tlLen++] = { ._id=tokIdent++, ._type=regExTokenType::rtClosure, ._min=0, ._max=1 };
          tl[tlLen++] = { ._id=tokIdent++, ._type=regExTokenType::rtGroupEnd                  };
        }
      }

      if (gs.cap == true)
        capIdent.pop();
    }

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
      tl[tlLen++] = { ._id=tokIdent++, ._type=regExTokenType::rtConcat };

    assert(tlLen <= tlMax);
  }

  //  Close the overall capture group.
  //tl[tlLen++] = makeGroupEnd(0);
  tl[tlLen++] = { ._id=tokIdent++, ._type=regExTokenType::rtGroupEnd, ._grpIdent=0 };

  return true;
}

}  //  merylutil::regex::v2
