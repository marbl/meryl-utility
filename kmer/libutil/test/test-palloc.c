#include <stdio.h>
#include <stdlib.h>

#include "util.h"

int
main(int argc, char **argv) {
  mt_s   *mtctx;
  int     i;

  psetdebug(2);
  psetblocksize(1024);

  palloc(2048);
  palloc(128);
  palloc(999);
  palloc(1);
  palloc(2);
  palloc(3);
  palloc(4);
  palloc(2056);
  palloc(8);
  palloc(2064);
  palloc(8);
  palloc(2072);
  palloc(8);

  pdumppalloc();

  pfree();

  fprintf(stderr, "----------------------------------------\n");

  psetblocksize(10240);

  palloc(2048);
  palloc(128);
  palloc(999);
  palloc(8);
  palloc(8);
  palloc(8);
  palloc(8);
  palloc(2056);
  palloc(8);
  palloc(2064);
  palloc(8);
  palloc(2072);
  palloc(8);

  pdumppalloc();

  pfree();

  psetdebug(0);
  psetblocksize(16 * 1024 * 1024);

  mtctx = mtInit(time(NULL));
  for (i=0; i<512 * 1024; i++)
    palloc(mtRandom32(mtctx) & 0xfff);
  psetdebug(1);
  pfree();

  return(0);
}

