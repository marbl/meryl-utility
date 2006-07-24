#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>  //  BYTE_ORDER

#include "/home/work/src/genomics/libutil/util.h"

//  Reports the byte order, writes words to files for testing.

#if 0

begin 644 test-alpha
.`@$$`P(!"`<&!00#`@$`
`
end
begin 644 test-i386
.`@$$`P(!"`<&!00#`@$`
`
end
begin 644 test-opteron
.`@$$`P(!"`<&!00#`@$`
`
end
begin 644 test-power
.`0(!`@,$`0(#!`4&!P@`
`
end

#endif


int
isBig1(void) {
  u64bit  l = u64bitONE;

  if (*((char *)(&l)) == 1)
    return(0);
  return(1);
}


//  supposedly due to Harbison and Steele
int
isBig2(void) {
  union {
    u64bit l;
    char c[sizeof(u64bit)];
  } u;

  u.l = u64bitONE;

#if 0
  fprintf(stderr, "%d%d%d%d%d%d%d%d\n",
          u.c[0], u.c[1], u.c[2], u.c[3],
          u.c[4], u.c[5], u.c[6], u.c[7]);
#endif

  if (u.c[0] == 1)   // LSB is first
    return(0);
  return(1);  //  MSB is first
}



int
main(int argc, char **argv) {
  u16bit   u16 = 0x0102;
  u32bit   u32 = u32bitNUMBER(0x01020304);
  u64bit   u64 = u64bitNUMBER(0x0102030405060708);

  fprintf(stderr, "BYTE_ORDER      = %d\n", BYTE_ORDER);

  fprintf(stderr, "  BIG_ENDIAN    = %d\n", BIG_ENDIAN);
  fprintf(stderr, "  LITTLE_ENDIAN = %d\n", LITTLE_ENDIAN);
  fprintf(stderr, "  PDP_ENDIAN    = %d\n", PDP_ENDIAN);

  fprintf(stderr, "isBig1()       = %d\n", isBig1());
  fprintf(stderr, "isBig2()       = %d\n", isBig2());

  if (argc == 1) {
    fprintf(stderr, "usage: %s [ write | read ] < source > check\n", argv[0]);
    exit(1);
  }

  if (strcmp(argv[1], "write") == 0) {
    fwrite(&u16, sizeof(u16bit), 1, stdout);
    fwrite(&u32, sizeof(u32bit), 1, stdout);
    fwrite(&u64, sizeof(u64bit), 1, stdout);
    return(0);
  }

  fread(&u16, sizeof(u16bit), 1, stdin);
  fread(&u32, sizeof(u32bit), 1, stdin);
  fread(&u64, sizeof(u64bit), 1, stdin);

#if 0
  //  swap bytes to convert u16
  u16 = (((u16 >> 8) & 0x00ff) |
         ((u16 << 8) & 0xff00));

  //  swap bytes, then swap words to convert u32
  u32 = (((u32 >> 24) & 0x000000ff) |
         ((u32 >>  8) & 0x0000ff00) |
         ((u32 <<  8) & 0x00ff0000) |
         ((u32 << 24) & 0xff000000));

  //  swap bytes, then flip words [0<->3, 1<->2] to convert u64
  u64 = (((u64 >> 24) & u64bitNUMBER(0x000000ff000000ff)) |
         ((u64 >>  8) & u64bitNUMBER(0x0000ff000000ff00)) |
         ((u64 <<  8) & u64bitNUMBER(0x00ff000000ff0000)) |
         ((u64 << 24) & u64bitNUMBER(0xff000000ff000000)));
  u64 = (((u64 >> 32) & u64bitNUMBER(0x00000000ffffffff)) |
         ((u64 << 32) & u64bitNUMBER(0xffffffff00000000)));
#endif

  if (u16 != 0x1234)
    fprintf(stderr, "u16 -- 0x%04x correct=0x%04x\n", u16, 0x1234);
  if (u32 != 0x12345678)
    fprintf(stderr, "u32 -- "u32bitHEX" correct="u32bitHEX"\n", u32, 0x12345678);
  if (u64 != u64bitNUMBER(0x1234567890abcdef))
    fprintf(stderr, "u64 -- "u64bitHEX" correct="u64bitHEX"\n", u64, u64bitNUMBER(0x1234567890abcdef));

  return(0);
}

