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

#include "strings.H"


int
main(int argc, char const **argv) {
  merylutil::regEx  re;
  int               arg = 1;
  int               err = 0;

  if (argc < 3) {
    fprintf(stderr, "usage: %s [-v] <regEx-string> <text-string> ...\n", argv[0]);
    exit(1);
  }

  for (arg=1; arg<argc; arg++) {
    if   (strcmp(argv[arg], "-v") == 0)
      re.enableVerbose();
    else
      break;
  }

  re.compile(argv[arg++]);

  for (; arg<argc; arg++) {
    fprintf(stdout, "MATCH: '%s'\n", argv[arg]);

    if (re.match(argv[arg]))
      fprintf(stdout, "  success! %d\n", re.isAccepted());
    else
      fprintf(stdout, "  failure. %d\n", re.isAccepted());

    for (uint64 ii=0; ii<re.numCaptures(); ii++)
      fprintf(stdout, "  %2lu %s %3lu-%3lu '%s'\n", ii,
              re.isValid(ii) ? "valid" : "inval", re.getBgn(ii), re.getEnd(ii), re.get(ii));
  }

  return 0;
}
