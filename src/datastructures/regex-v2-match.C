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


static bool verbose = true;


bool
regEx::match(char const *query) {

  if (verbose) {
    fprintf(stderr, "\n");
    fprintf(stderr, "MATCHING\n");
    fprintf(stderr, "\n");
  }

  //  Allocate and clear space for capture results.  capsLen is computed in
  //  parsing.

  resizeArray(bgnP, endP, caps, 0, capsMax, capsLen);

  for (uint64 ii=0; ii<capsLen; ii++) {
    bgnP[ii] = uint64max;
    endP[ii] = uint64max;
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
  bool         lastacc = false;
  bool         thisacc = false;

  lastacc = thisacc = followStates(re._bgn, sA, sAlen);

  for (uint64 pp=0; query[pp]; pp++) {
    char ch = query[pp];   //  Letter to process.

    if (sAlen == 0) {   //  If no states to explore, quit and use acceptance of the
      if (verbose)
        fprintf(stderr, "BREAK because no states to explore.\n");
      //accept = lastacc;
      break;            //  last letter - this lets us accept patterns shorter than the text.
    }

    sBlen   = 0;
    lastacc = thisacc;
    thisacc = false;

    if (verbose) {
      fprintf(stderr, "\n");
      fprintf(stderr, "ADVANCE on character '%c' with %u states to explore\n", ch, sAlen);
    }

    //  valgrind ../build/bin/regex '({p}abc(123)(456)def)hij' abc123hij
    //  (456) isn't being treated as an atom in the prefix group

    //  Explore each of the states in our list.
    //  If the state has a match to letter 'ch' follow the link and add
    //  either the node it points to, or the nodes via lambda edges.  Remember
    //  if any of those are accepting states.

    //  This fails to claim acceptance if we reach the end of the regex before the
    //  end of the string.

    for (uint64 ii=0; ii<sAlen; ii++) {
      regExState *S = sA[ii];
      regExToken  T = sA[ii]->_tok;

      if (T.isMatch(ch)) {
        thisacc |= followStates(S->_match, sB, sBlen);
        if (verbose)
          fprintf(stderr, "        on character '%c' match to %lu with accept %d\n", ch, S->_id, thisacc);
      } else {
      }

      if (T.isMatch(ch) && (T._grpIdent < capsLen) && (bgnP[T._grpIdent] == uint64max))   bgnP[T._grpIdent] = pp;
      if (T.isMatch(ch) && (T._grpIdent < capsLen))                                       endP[T._grpIdent] = pp + 1;
    }

    if (verbose)
      fprintf(stderr, "        on character '%c' with %u states to explore next lastacc:%d thisacc:%d\n", ch, sBlen, lastacc, thisacc);

    std::swap(sA,    sB);
    std::swap(sAlen, sBlen);
  }

  //  All done.  CAses for finishing the loop:
  //    Matched to end of string:       accept == 
  //    Matched to all but last letter: accept incorrect
  //    Matched to not last letter:     accept == lastacc

  accept = lastacc;

  if (verbose)
    fprintf(stderr, "lens %u %u acc %d\n", sAlen, sBlen, accept);
  delete [] states;

  //  If we ended up in the terminal state, declare the match a success.


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
