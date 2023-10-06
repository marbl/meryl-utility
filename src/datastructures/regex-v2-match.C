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

bool
regEx::match(char const *query) {

  if (vMatch) {
    fprintf(stderr, "\n");
    fprintf(stderr, "MATCHING\n");
    fprintf(stderr, "\n");
  }

  //  Allocate and clear space for capture results.  capsLen is computed in
  //  parsing.

  resizeArray(bgnP, endP, caps, 0, capsMax, capsLen);

  for (uint64 ii=0; ii<capsLen; ii++) {
    bgnP[ii] = uint64max;
    endP[ii] = 0;
    caps[ii] = nullptr;
  }

  //  Initialze the list of next-states with all that are reachable via a
  //  lambda from the root node.
  //
  //  Then find a new set of next-states that are reachable from any state in
  //  the current list by following an edge for the current query letter.
  //
  //  If a match is found and we're in a capture group, remember
  //  the min/max string coords for that capture group.

  regExState  **states = new regExState * [rsLen + rsLen];

  regExState **sA = states + 0;      uint64 sAlen = 0;
  regExState **sB = states + rsLen;  uint64 sBlen = 0;

  bool         accept  = false;

  accept = followStates(re._bgn, sA, sAlen);

  for (uint64 pp=0; query[pp]; pp++) {
    char ch = query[pp];   //  Letter to process.

    if (vMatch) {
      fprintf(stderr, "\n");
      fprintf(stderr, "ADVANCE on character '%c' with %u states to explore\n", ch, sAlen);
    }

    //  Explore each of the states in our list.
    //
    //  If the state doesn't match our letter, skip it.
    //
    //  has a match to letter 'ch' follow the link and add
    //  either the node it points to, or the nodes via lambda edges.  Remember
    //  if any of those are accepting states.

    bool  hasMatches = false;
    bool  newAccept  = false;

    for (uint64 ii=0; ii<sAlen; ii++) {
      regExState *S = sA[ii];                             //  The state to explore.
      regExToken  T = sA[ii]->_tok;                       //  The token we need to match.
      uint64      g = sA[ii]->_tok._grpIdent;             //  Capture group associated with this state.

      assert(g < capsLen);

      if (T.isMatch(ch) == false)                         //  No match, nothing further to do.
        continue;

      hasMatches = true;                                  //  Remember some state had a match, and follow
      newAccept |= followStates(S->_match, sB, sBlen);    //  links from this state out.

      if (vMatch)
        fprintf(stderr, "        on character '%c' match to %lu with accept %d\n", ch, S->_id, newAccept);

      bgnP[0] = std::min(bgnP[0], pp);                    //  Always add matches to the first capture group.
      endP[0] = std::max(endP[0], pp+1);

      bgnP[g] = std::min(bgnP[g], pp);                    //  And add matches to any secondary capture groups.
      endP[g] = std::max(endP[g], pp+1);
    }  //  Over all states in the current multiset.

    if (sBlen == 0) {
      if (vMatch)
        fprintf(stderr, "        on character '%c' - STOP:  no next states.  accept:%d\n", ch, accept);
    }
    if (hasMatches == false) {
      if (vMatch)
        fprintf(stderr, "        on character '%c' - STOP:  no matches.      accept:%d\n", ch, accept);
      break;
    }

    accept = newAccept;                                   //  Matches found!  Remember the new acceptance status.

    if (vMatch)
      fprintf(stderr, "        on character '%c' with %u states to explore next accept:%d\n", ch, sBlen, accept);

    std::swap(sA,    sB);                                 //  Swap state lists so we iterate over the states
    std::swap(sAlen, sBlen);  sBlen = 0;                  //  we discovered above in the next iteration.
  }  //  Over all letters in query.

  //  All done.  CAses for finishing the loop:
  //    Matched to end of string:             accept == thisaccept
  //    Matched to all but last letter:       accept == thisaccept     BREAK above
  //    Matched to not last letter:           accept == thisaccept     BREAK above
  //    Matched to end of pattern via lambda: accept == lastaccept     BREAK above

  //accept = thisacc;

  //  Copy the captures to individual strings.

  for (uint64 ii=0; ii<capsLen; ii++) {
    if     ((bgnP[ii] == uint64max) && (endP[ii] == uint64max))
      bgnP[ii] = endP[ii] = 0;
    else if (bgnP[ii] == uint64max)
      assert(0);  //  bgnP not set but endP is
    else if (endP[ii] == uint64max)
      assert(0);  //  endP not set but bgnP is
  }

  capstorLen = 0;
  for (uint64 ii=0; ii<capsLen; ii++)
    capstorLen += endP[ii] - bgnP[ii] + 1;

  resizeArray(capstor, 0, capstorMax, capstorLen, _raAct::doNothing);

  capstorLen = 0;
  for (uint64 ii=0; ii<capsLen; ii++) {
    caps[ii] = capstor + capstorLen;

    for (uint64 ss=bgnP[ii]; ss<endP[ii]; ss++)
      capstor[capstorLen++] = query[ss];

    capstor[capstorLen++] = 0;
  }

  assert(capstorLen <= capstorMax);


  return accept;
}

}  //  merylutil::regex::v2
