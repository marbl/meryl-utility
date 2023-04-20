TARGET   := testHTSlib
SOURCES  := testHTSlib.c

SRC_INCDIRS := .. ../utility ../utility/htslib

TGT_LDFLAGS := -L${TARGET_DIR}/lib -lz -llzma -lbz2 -lcurl
TGT_LDLIBS  := -l${MODULE}
TGT_PREREQS := lib${MODULE}.a
