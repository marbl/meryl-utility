#include <stdio.h>

#include "bri.h"

int
main(void) {
  u32bit   errors = 0;
  u32bit   u3 = -1;
  s32bit   s3 = -1;
  u64bit   u6 = -1;
  s64bit   s6 = -1;

  if (sizeof(u32bit) != 4)
    fprintf(stderr, "u32bit has %ld bytes (should be 4)!\n", sizeof(u32bit)), errors++;

  if (sizeof(u64bit) != 8)
    fprintf(stderr, "u64bit has %ld bytes (should be 8)!\n", sizeof(u64bit)), errors++;

  if (u3 < 0)
    fprintf(stderr, "u32bit is signed (should be unsigned)!\n"), errors++;

  if (s3 > 0)
    fprintf(stderr, "s32bit is unsigned (should be signed)!\n"), errors++;

  if (u6 < 0)
    fprintf(stderr, "u64bit is signed (should be unsigned)!\n"), errors++;

  if (s6 > 0)
    fprintf(stderr, "s64bit is unsigned (should be signed)!\n"), errors++;

  return(errors);
}

