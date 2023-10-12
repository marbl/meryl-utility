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



//  Explore edges out of current to find nodes to explore next.
//
//  If we're not at a node, do nothing.
//  If this node has a match symbol, we want to expore it.
//  If not, follow lambda edges to find other match nodes.
//   - by construction, a node has either a match link or lambda links.
//
void
regEx::findNodes(uint64      previous,
                 regExState *current,
                 uint64      currentPos,
                 regExMatch *&match, uint64 &matchLen, uint64 &matchMax,
                 uint64 *sB, uint64 &sBlen) {

  if (current == nullptr) {
  }
  else if ((current->_match) ||               //  Save 'current' for exploration if it
           (current->_accepting)) {           //  has a match node OR is accepting.
    match[matchLen]._state     = current;     //    Link to state to explore.
    match[matchLen]._prevMatch = previous;    //    Link to state we came from.
    match[matchLen]._stringIdx = currentPos;  //    Position in string we're exploring.

    sB[sBlen++] = matchLen++;

    assert(current->_lambda[0] == nullptr);
    assert(current->_lambda[1] == nullptr);
  }
  else {
    findNodes(previous, current->_lambda[0], currentPos, match, matchLen, matchMax, sB, sBlen);
    findNodes(previous, current->_lambda[1], currentPos, match, matchLen, matchMax, sB, sBlen);
  }
}


//  Return true if we can get to an accepting state following lambda edges.
bool
regEx::isAccepting(regExState *current) {
  if (current == nullptr)
    return false;
  return current->_accepting | isAccepting(current->_lambda[0]) | isAccepting(current->_lambda[1]);
}


  //  Initialze the list of next-states with all that are reachable via a
  //  lambda from the root node.
  //
  //  Then find a new set of next-states that are reachable from any state in
  //  the current list by following an edge for the current query letter.
  //
  //  If a match is found and we're in a capture group, remember
  //  the min/max string coords for that capture group.

bool
regEx::match(char const *query) {

  if (rsLen == 0)
    return false;

  if (vMatch) {
    fprintf(stderr, "\n");
    fprintf(stderr, "MATCHING\n");
    fprintf(stderr, "\n");
  }

  uint64       matchLen = 0;
  uint64       matchMax = 1024;
  regExMatch  *match    = new regExMatch [matchMax];

  uint64  *active = new uint64 [rsLen + rsLen];
  uint64  *sA = active + 0;      uint64 sAlen = 0;
  uint64  *sB = active + rsLen;  uint64 sBlen = 0;

  uint64   acceptLen = 0;
  uint64  *accept    = new uint64 [rsLen + rsLen];


  //  Find the initial set of nodes to explore by following links out of re._bgn.
  findNodes(uint64max, re._bgn, 0, match, matchLen, matchMax, sA, sAlen);



  for (uint64 pp=0; query[pp]; pp++) {
    char ch = query[pp];   //  Letter to process.

    if (vMatch)
      fprintf(stderr, "\nADVANCE ch '%c' with %lu states to explore\n", ch, sAlen);

    for (uint64 ii=0; ii<sAlen; ii++) {                //  Over all the nodes in the set
      regExState *S  = match[ sA[ii] ]._state;         //  to explore....
      regExToken  T  = match[ sA[ii] ]._state->_tok;

      if (S->_match == nullptr)                        //  Skip nodes with no match state; these
        continue;                                      //  are (hopefully) accepting states!

      bool  m = T.isMatch(ch);                         //  Test if ch matches what this node is expecting.

      if (vMatch)
        fprintf(stderr, "   [%02lu] -> '%c' -> [%02lu] %s %s\n",
                S->_id, ch, S->_match->_id, m ? "PASS" : "fail", T.display());

      if (m == false)                                  //  Nope, ch does NOT match what this node is expecting.
        continue;

      findNodes(sA[ii], S->_match, pp+1,               //  Follow edges out of the match edge to find more
                match, matchLen, matchMax, sB, sBlen); //  nodes to explore.
    }

    if (vMatch)
      fprintf(stderr, "        ch '%c' with %lu states to explore next\n", ch, sBlen);

    if (sBlen == 0)                                    //  Either no matches out of here, or no more states left.
      break;                                           //  We're done.  Retain the last list of nodes.

    std::swap(sA,    sB);                              //  Swap state lists so we iterate over the states
    std::swap(sAlen, sBlen);  sBlen = 0;               //  we discovered above in the next iteration.
  }  //  Over all letters in query.


  //  Decide if the last set of states was accepting or not.
  bool acc = false;
  fprintf(stderr, "Final states:\n");
  for (uint64 ii=0; ii<sAlen; ii++) {
    regExState *S  = match[ sA[ii] ]._state;
    regExToken  T  = match[ sA[ii] ]._state->_tok;

    fprintf(stderr, "   [%02lu] accept:%d %s\n", S->_id, isAccepting(S), T.display());

    acc |= isAccepting(S);
  }

  //  By construction (see the end of regex-v2-parse.C) there is always at least
  //  one capture group, and we have already allocated space for storing those.
  //
  //  Allocate space for an array of all empty strings, then set
  //  the capture group output to be all empty.

  resizeArray(capstor, 0, capstorMax, capsLen, _raAct::doNothing);

  for (uint32 c=0; c<capsLen; c++) {
    bgnP[c]          = 0;
    endP[c]          = 0;
    lenP[c]          = 0;
    caps[c]          = (c == 0) ? (capstor) : (caps[c-1] + 1);
    caps[c][0]       = 0;
  }

  if (sAlen == 0)    //  Bail if there are no states
    return false;    //  left.  We didn't match.


  //  Compute the actual length of each capture, then allocate
  //  space for results.

  capstorLen = capsLen;

  for (uint64 mi = sA[0]; mi != uint64max; mi = match[mi]._prevMatch) {
    for (auto c : match[mi]._state->_tok._capGroups)
      lenP[c]    += 1;

    capstorLen += match[mi]._state->_tok._capGroups.size();
  }

  resizeArray(capstor, 0, capstorMax, capstorLen, _raAct::doNothing);

  for (uint32 c=0; c<capsLen; c++) {   //  Update string pointers to new memory.
    bgnP[c]          = (lenP[c] > 0) ? uint64max : 0;   //  If letters, set bgn to maximum value so
    endP[c]          = (lenP[c] > 0) ? 0         : 0;   //  that min() below works.
    caps[c]          = (c == 0) ? (capstor) : (caps[c-1] + lenP[c-1] + 1);
    caps[c][0]       = 0;
    caps[c][lenP[c]] = 0;
  }

  //  Iterate over the match path again and copy letters to outputs.

  for (uint64 mi = sA[0]; mi != uint64max; mi = match[mi]._prevMatch) {
    regExState  *S = match[mi]._state;
    uint64       p = match[mi]._prevMatch;
    uint64      pp = match[mi]._stringIdx;
    char        ch = query[pp];

    fprintf(stderr, "accept[%03lu] query[%03lu]='%c'  %s\n", mi, pp, ch, S->_tok.display());

    for (auto c : S->_tok._capGroups) {
      bgnP[c] = std::min(bgnP[c], pp);
      endP[c] = std::max(endP[c], pp+1);

      caps[c][--lenP[c]] = ch;

      fprintf(stderr, "            group:%lu '%c' %3lu-%3lu len %3lu\n", c, ch, bgnP[c], endP[c], lenP[c]);
    }
  }

  for (uint32 c=0; c<capsLen; c++)
    fprintf(stderr, "RESULTS:  %02u %02lu-%02lu '%s'\n", c, bgnP[c], endP[c], caps[c]);

  delete [] active;
  delete [] match;
  delete [] accept;

  return acc;
}

}  //  merylutil::regex::v2
