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
#include "strings.H"

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
  return current->_accepting || isAccepting(current->_lambda[0]) || isAccepting(current->_lambda[1]);
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

  uint64       aClen = 0;
  uint64      *aC    = new uint64 [matchMax];

  uint64      *sA = new uint64 [rsLen + rsLen];      //  Bulk storage of state indices
  uint64      *sO = sA + 0;      uint64 sOlen = 0;   //  Old states to iterate over
  uint64      *sN = sA + rsLen;  uint64 sNlen = 0;   //  New states discoered

  //  Find the initial set of nodes to explore by following links out of re._bgn.
  findNodes(uint64max, re._bgn, 0, match, matchLen, matchMax, sN, sNlen);

  uint64 pp    = 0;
  uint64 ppMax = strlen(query) + 1;

  do {
    char ch = query[pp++];                               //  Copy ch to match, move to next for next round.

    std::swap(sN,    sO);                                //  Swap state lists so we iterate over the states
    std::swap(sNlen, sOlen);  sNlen = 0;                 //  we discovered last iteration (sN) now (sO).

    if (vMatch)
      fprintf(stderr, "\nADVANCE ch '%s' with %lu states to explore\n", displayLetter(ch), sOlen);

    for (uint64 ii=0; ii<sOlen; ii++) {                  //  Over all the nodes in the set
      regExState *S  = match[ sO[ii] ]._state;           //  to explore....
      regExToken  T  = match[ sO[ii] ]._state->_tok;

      if (S->_match == nullptr)                          //  Skip nodes with no match state; these
        continue;                                        //  are (hopefully) accepting states!

      bool  m = T.isMatch(ch);                           //  Test if ch matches what this node is expecting.

      if (vMatch)
        fprintf(stderr, "   [%02lu] -> '%s' -> [%02lu] %s %s\n",
                S->_id, displayLetter(ch), S->_match->_id, m ? "PASS" : "fail", T.display());

      if (m)                                             //  If ch does match, follow edges out of the
        findNodes(sO[ii], S->_match, pp,                 //  match edge to find more nodes to explore.
                  match, matchLen, matchMax, sN, sNlen); //  NB: pp is already at the next letter.
    }

    for (uint64 ii=0, reset=1; ii<sNlen; ii++) {         //  Rebuild accepting state list if there
      if (match[ sN[ii] ]._state->_accepting == true) {  //  are accepting states in the next round.
        if (reset)                                       //  
          aClen = reset = 0;                             //  Over each state, if it is an accepting
        aC[ aClen++ ] = sN[ii];                          //  state, reset the accepting list if
      }                                                  //  this is the first accepting state,
    }                                                    //  then append the accepting state.

    if (vMatch)
      fprintf(stderr, "        ch '%s' with %lu states to explore next; aClen %lu\n", displayLetter(ch), sNlen, aClen);
  } while ((sNlen > 0) && (pp < ppMax));


  //  Display the final states.

  if (vMatch) {
    regExState *S;

    fprintf(stderr, "\nFinal state report:\n");
    fprintf(stderr, "  New states:\n");
    for (uint64 ii=0; (ii<sNlen) && (S = match[ sN[ii] ]._state); ii++)
      fprintf(stderr, "    [%02lu] accept:%d %s\n", S->_id, isAccepting(S), S->_tok.display());
    if (sNlen == 0)
      fprintf(stderr, "    none\n");

    fprintf(stderr, "  Old states:\n");
    for (uint64 ii=0; (ii<sOlen) && (S = match[ sO[ii] ]._state); ii++)
      fprintf(stderr, "    [%02lu] accept:%d %s\n", S->_id, isAccepting(S), S->_tok.display());
    if (sOlen == 0)
      fprintf(stderr, "    none\n");

    fprintf(stderr, "  Accepting states:\n");
    for (uint64 ii=0; (ii<aClen) && (S = match[ aC[ii] ]._state); ii++)
      fprintf(stderr, "    [%02lu] accept:%d %s\n", S->_id, isAccepting(S), S->_tok.display());
    if (aClen == 0)
      fprintf(stderr, "    none\n");

    fprintf(stderr, "\n");
  }

  //  By construction (see the end of regex-v2-parse.C) there is always at
  //  least one capture group, and we have already allocated space for
  //  storing those results (bgnP, endP, etc).
  //
  //  Allocate space for an array of all empty strings, then set the capture
  //  group output to be all empty.

  resizeArray(capstor, 0, capstorMax, capsLen, _raAct::doNothing);

  for (uint32 c=0; c<capsLen; c++) {
    bgnP[c]          = 0;
    endP[c]          = 0;
    lenP[c]          = 0;
    caps[c]          = (c == 0) ? (capstor) : (caps[c-1] + 1);
    caps[c][0]       = 0;
  }

  accepting = (aClen > 0);

  if (accepting == false)    //  Bail; makes the logic below easier.
    goto exitmatch;          //

  //  Iterate over the match path to compute the actual length of each
  //  capture, then allocate space for results.

  capstorLen = capsLen;

  for (uint64 mi = aC[0]; mi != uint64max; mi = match[mi]._prevMatch) {
    for (auto c : match[mi]._state->_tok._capGroups)
      lenP[c] += 1;

    capstorLen += match[mi]._state->_tok._capGroups.size();
  }

  resizeArray(capstor, 0, capstorMax, capstorLen, _raAct::doNothing);

  //  Update string pointers to the new memory, set bgnP and endP to max/min
  //  respectively so we can use min() and max() respectively to track the
  //  match positions.

  for (uint32 c=0; c<capsLen; c++) {
    bgnP[c]          = (lenP[c] > 0) ? uint64max : 0;
    endP[c]          = (lenP[c] > 0) ? 0         : 0;
    caps[c]          = (c == 0) ? (capstor) : (caps[c-1] + lenP[c-1] + 1);
    caps[c][0]       = 0;
    caps[c][lenP[c]] = 0;
  }

  //  Iterate over the match path again and copy letters to outputs.

  if (vMatch)
    fprintf(stderr, "\nCopy strings to groups.\n");

  for (uint64 mi = aC[0]; mi != uint64max; mi = match[mi]._prevMatch) {
    regExState  *S = match[mi]._state;
    uint64       p = match[mi]._prevMatch;
    uint64      pp = match[mi]._stringIdx;
    char        ch = query[pp];

    if (vMatch)
      fprintf(stderr, "  [%03lu] query[%03lu]='%s'  %s\n", mi, pp, displayLetter(ch), S->_tok.display());

    for (auto c : S->_tok._capGroups) {
      bgnP[c] = std::min(bgnP[c], pp);
      endP[c] = std::max(endP[c], pp+1);

      caps[c][--lenP[c]] = ch;

      if (vMatch)
        fprintf(stderr, "        group[%03lu] -> %3lu-%3lu '%s'\n", c, bgnP[c], endP[c], displayString(caps[c] + lenP[c]));
    }
  }

  //  Bubble groups with matches to the front of the list.

  for (uint64 ii=0, jj=1; ii<capsLen && jj<capsLen; ii++) {
    if (caps[ii][0] != 0)                        //  Do nothing if ii has data, otherwise
      continue;                                  //  swap ii with the first jj with data.

    for (jj=ii+1; ((jj < capsLen) &&             //  Find first jj with data.
                   (caps[jj][0] == 0)); jj++)    //
      ;
         
    if (jj < capsLen) {                          //  If such a jj exists, swap
      std::swap(bgnP[ii], bgnP[jj]);             //  into the empty ii slot.
      std::swap(endP[ii], endP[jj]);
      std::swap(lenP[ii], lenP[jj]);
      std::swap(caps[ii], caps[jj]);
    }
  }

  //  Cleanup and return.

 exitmatch:
  delete [] aC;
  delete [] sA;
  delete [] match;

  return accepting;
}

}  //  merylutil::regex::v2
