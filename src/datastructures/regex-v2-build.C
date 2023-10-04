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

#include <set>
#include <map>
#include <algorithm>

namespace merylutil::inline regex::inline v2 {

static bool verbose = true;


regExExpr
regEx::concat(regExState *rs, uint64 &rsLen, regExExpr a, regExExpr b) {

  if (verbose)
    fprintf(stderr, "concat() %lu..%lu -> %lu..%lu:\n",
            a._bgn->_id, a._end->_id,
            b._bgn->_id, b._end->_id);

  a._end->addEpsilon(b._bgn);

  return regExExpr(a._bgn, b._end);
}


regExExpr
regEx::alternate(regExState *rs, uint64 &rsLen, regExExpr a, regExExpr b) {
  regExState  *bgn = &rs[rsLen++];
  regExState  *end = &rs[rsLen++];

  if (verbose)
    fprintf(stderr, "alternate() %lu -> %lu..%lu | %lu..%lu -> %lu\n",
            bgn->_id,
            a._bgn->_id, a._end->_id,
            b._bgn->_id, b._end->_id,
            end->_id);

  bgn->addEpsilon(a._bgn);
  bgn->addEpsilon(b._bgn);

  a._end->addEpsilon(end);
  b._end->addEpsilon(end);

  return regExExpr(bgn, end);
}



void
findReachable(regExState *a, std::set<uint64> &reachable) {
  regExState *t;

  if ((a == nullptr) || (reachable.contains(a->_id) == true))
    return;

  reachable.insert(a->_id);

  t = a->_match;      if (t)  findReachable(t, reachable);
  t = a->_lambda[0];  if (t)  findReachable(t, reachable);
  t = a->_lambda[1];  if (t)  findReachable(t, reachable);
}


regExExpr
regExExpr::duplicate(regExState *rs, uint64 &rsLen) {
  std::set<uint64>         reachable;
  std::map<uint64,uint64>  idMap;
  regExExpr                ne;

  if (verbose)
    fprintf(stderr, "duplicate() %lu..%lu\n", _bgn->_id, _end->_id);

  //  Find the reachable nodes from the beginning of this expression
  findReachable(_bgn, reachable);

  //  Build a map from original node ident to new node ident.
  //  (but do not otherwise initialize nodes)
  for (auto a : reachable) {
    idMap[a] = rsLen++;
    if (verbose)
      fprintf(stderr, "duplicate()    map %lu -> %lu\n", a, idMap[a]);
  }

  //  Duplicate nodes.
  for (auto a : reachable) {
    regExState *o = &rs[       a  ];
    regExState *d = &rs[ idMap[a] ];

    //>_id        = o->_id
    d->_accepting = o->_accepting;
    d->_tok       = o->_tok;

    if (o->_match)       d->_match     = &rs[ idMap[o->_match->_id    ] ];
    if (o->_lambda[0])   d->_lambda[0] = &rs[ idMap[o->_lambda[0]->_id] ];
    if (o->_lambda[1])   d->_lambda[1] = &rs[ idMap[o->_lambda[1]->_id] ];
  }

  ne._bgn = &rs[ idMap[_bgn->_id] ];
  ne._end = &rs[ idMap[_end->_id] ];

  if (verbose)
    fprintf(stderr, "duplicate() %lu..%lu into %lu..%lu\n", _bgn->_id, _end->_id, ne._bgn->_id, ne._end->_id);

  return ne;
}


//  Cases:
//    1) min=0, max=uint64max - *     - the classic closure
//    2) min=0, max=1         - ?     - zero or one
//    3) min=1, max=uint64max - +     - one or more
//    4) min>0, max<uint64max - {x,y} - general case
//
regExExpr
regEx::closure(regExState *rs, uint64 &rsLen, regExExpr a, uint64 min, uint64 max) {
  regExState  *bgn = &rs[rsLen++];
  regExState  *end = &rs[rsLen++];

  uint64       nnn =       min;
  uint64       mmm = max - min;
  regExExpr   *nnndups = nullptr;
  regExExpr   *mmmdups = nullptr;

  if (verbose)
    fprintf(stderr, "closure() %lu -> %lu..%lu -> %lu\n",
            bgn->_id,
            a._bgn->_id, a._end->_id,
            end->_id);

  if ((min == 0) && (max ==uint64max)) {
    bgn->addEpsilon(end);
    bgn->addEpsilon(a._bgn);

    a._end->addEpsilon(end);
    a._end->addEpsilon(a._bgn);
    goto finishclosure;
  }

  if ((min == 0) && (max == 1)) {
    bgn->addEpsilon(end);
    bgn->addEpsilon(a._bgn);

    a._end->addEpsilon(end);
    //_end->addEpsilon(a._bgn);   //  Just like '*' but can't traverse multiple times.
    goto finishclosure;
  }

  //  Add 'nnn' copies of 'a', chained together.

  nnndups = new regExExpr [nnn];
  for (uint64 ii=0; ii<nnn; ii++)
    nnndups[ii] = a.duplicate(rs, rsLen);

  bgn->addEpsilon(nnndups[0]._bgn);                    //  Enter the chain of 'a' copies

  for (uint64 ii=0; ii<nnn-1; ii++)
    nnndups[ii]._end->addEpsilon(nnndups[ii+1]._bgn);  //  Leave 'a' to next copy.

  nnndups[nnn-1]._end->addEpsilon(end);                //  Leave (last copy of) 'a' to accepting.

  //  If 'max' is infinite, add 'a' itself as a normal closure.

  if (max == uint64max) {
    //ndups[nnn-1]._end->addEpsilon(end);               //   Skip all of 'a'; exists already.
    nnndups[nnn-1]._end->addEpsilon(a._bgn);            //   Enter 'a'.

    a._end->addEpsilon(end);                            //   Leave 'a' to accepting.
    a._end->addEpsilon(a._bgn);                         //   Leave 'a' back to the start of it.
  }

  //  Otherwise, add 'mmm' copies of a, chained together, but allowing any one
  //  to jump to the accepting state.

  else {
    mmmdups = new regExExpr [mmm];
    for (uint64 ii=0; ii<mmm; ii++)
      mmmdups[ii] = a.duplicate(rs, rsLen);

    //ndups[nnn-1]._end->addEpsilon(end);               //  Skip all of 'a'; exists already.
    nnndups[nnn-1]._end->addEpsilon(mmmdups[0]._bgn);   //  Enter the chain of 'a' copies

    for (uint64 ii=0; ii<mmm-1; ii++) {
      mmmdups[ii]._end->addEpsilon(mmmdups[ii+1]._bgn);  //  Leave 'a' to next copy.
      mmmdups[ii]._end->addEpsilon(end);                 //  Leave 'a' to accepting.
    }

    mmmdups[mmm-1]._end->addEpsilon(end);                //  Leave (last copy of) 'a' to accepting.
  }

 finishclosure:
  return regExExpr(bgn, end);
}


regExExpr
regEx::symbol(regExState *rs, uint64 &rsLen, regExToken tok) {
  regExState  *bgn = &rs[rsLen++];
  regExState  *mat = &rs[rsLen++];
  regExState  *end = &rs[rsLen++];

  if (verbose)
    fprintf(stderr, "symbol() %lu -> %lu -> %lu for %s\n", bgn->_id, mat->_id, end->_id, tok.display());

  bgn->addEpsilon(mat);
  mat->addMatch(tok, end);

  return regExExpr(bgn, end);
}


//  Add state 'in' to th elist of states to explore if it
//  is a non-empty state.  If it is an empty state, recurse
//  through its lambda links.
bool
regEx::followStates(regExState *in, regExState **sB, uint64 &sBlen) {
  bool  acc = in->_accepting;

  if (in->_lambda[0] != nullptr)  acc |= followStates(in->_lambda[0], sB, sBlen);
  if (in->_lambda[1] != nullptr)  acc |= followStates(in->_lambda[1], sB, sBlen);

  if (in->_match     != nullptr)  sB[sBlen++] = in;

  return acc;
}



bool
regEx::build(void) {
  uint64       olLen = 0;

  //  Allocate states, one for each token in the token list, then initialize
  //  them with a unique ident (for debugging).

  rsMax = 1024;   //  List of allocted states
  rsLen = 0;
  rs    = new regExState [rsMax];

  for (uint64 ii=0; ii<rsMax; ii++)
    rs[ii]._id = ii;

  if (verbose) {
    fprintf(stderr, "\n");
    fprintf(stderr, "BUILDING\n");
    fprintf(stderr, "\n");
  }

  //  Iterate over all states.  Push charater-matching states onto the stack
  //  as a simple expression, then pop off the required number of expressions
  //  when an operator shows up, merge them according to the operator, and
  //  push the new expression onto the stack.

  merylutil::stack<regExExpr>  st;

  for (uint64 tt=0; tt<tlLen; tt++) {
    regExExpr a;
    regExExpr b;

    switch (tl[tt]._type) {
      case regExTokenType::rtAlternation:
        a = st.pop();
        b = st.pop();
        st.push(alternate(rs, rsLen, b, a));
        break;

      case regExTokenType::rtClosure:
        a = st.pop();
        st.push(closure(rs, rsLen, a, tl[tt]._min, tl[tt]._max));
        break;

      case regExTokenType::rtConcat:
        a = st.pop();
        b = st.pop();
        st.push(concat(rs, rsLen, b, a));
        break;

      case regExTokenType::rtCharClass:
        st.push(symbol(rs, rsLen, tl[tt]));
        break;

      default:
        assert(0);
        break;
    }
  }

  if (verbose)
    fprintf(stderr, "stLen %u rsLen %u\n", st.depth(), rsLen);

  assert(st.depth() == 1);
  re = st.pop();

  re._end->_accepting = true;

#if 0
  for (uint64 ii=0; ii<rsLen; ii++)
    fprintf(stderr, "rs[%lu] - id:%lu match:%lu lambda:%lu:%lu\n",
            ii,
            rs[ii]._id,
            rs[ii]._match     ? rs[ii]._match->_id     : -1,
            rs[ii]._lambda[0] ? rs[ii]._lambda[0]->_id : -1,
            rs[ii]._lambda[1] ? rs[ii]._lambda[1]->_id : -1);
#endif

  return true;
}



}  //  merylutil::regex::v2
