//
//  Automagically generated -- DO NOT EDIT!
//  See libbri/alphabet-generate.c for details.
//

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char   whitespaceSymbol[256];
extern unsigned char   toLower[256];
extern unsigned char   toUpper[256];
extern unsigned char   compressSymbol[256];
extern unsigned char   validSymbol[256];
extern unsigned char   decompressSymbol[256];
extern unsigned char   complementSymbol[256];
extern unsigned char   validCompressedSymbol[256];
extern unsigned char   IUPACidentity[128][128];

void initCompressionTablesForACGTSpace(void);
void initCompressionTablesForColorSpace(void);

#ifdef __cplusplus
}
#endif
