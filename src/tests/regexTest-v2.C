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

  if (argc < 3) {
    fprintf(stderr, "usage: %s <regEx-string> <text-string> ...\n", argv[0]);
    exit(1);
  }

  merylutil::regEx  re;

  re.enableVerbose();
  re.construct(argv[1]);

  for (uint32 arg=2; arg<argc; arg++) {
    fprintf(stdout, "MATCH: '%s'\n", argv[arg]);

    if (re.match(argv[arg]))
      fprintf(stdout, "  success!\n");
    else
      fprintf(stdout, "  failure\n");

    for (uint64 ii=0; ii<re.numCaptures(); ii++)
      fprintf(stdout, "  %2lu %3lu-%3lu '%s'\n", ii, re.getCaptureBgn(ii), re.getCaptureEnd(ii), re.getCapture(ii));
  }

  return 0;
}
