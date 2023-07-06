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
//  ^     -- matches start of line
//  $     -- matches end of line (either end of string or newline)
//  .     -- matches any character (except newline)
//  []    -- matches any of a set of characters
//  [^]   -- matches any except a set of characters
//  \n    -- BRE only, matches a previously matched group
//  *     -- matches previous element zero or more times (append ? to match minimally)
//  {m,n} -- matches previous element at least m and at most n times (append ? to match minimally)
//
//  ?     -- matches previous element zero or one times (append ? to match minimally)
//  +     -- matches previous element at lest once (append ? to match minimally)
//  |     -- matches either the expression before or the expression after
//
//  ()    -- grouping and capturing
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

#endif  //  MERYLUTIL_REGEX_H
