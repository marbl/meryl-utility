TARGET   := intervalListTest
SOURCES  := intervalListTest.C

SRC_INCDIRS := .. ../utility

TGT_LDFLAGS := -L${TARGET_DIR}/lib
TGT_LDLIBS  := -lcanu
TGT_PREREQS := libcanu.a

SUBMAKEFILES :=
