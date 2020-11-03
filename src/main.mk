MODULE       :=    meryl-utility
TARGET       := libmeryl-utility.a
SOURCES      := utility/runtime.C \
                \
                utility/align-ksw2-extz.C \
                utility/align-ksw2-extz2-sse.C \
                utility/align-ksw2-driver.C \
                utility/align-ssw.C \
                utility/align-ssw-driver.C \
                utility/align-parasail-driver.C \
                utility/edlib.C \
                \
                utility/files.C \
                utility/files-buffered.C \
                utility/files-compressed.C \
                utility/files-memoryMapped.C \
                \
                utility/logging.C \
                \
                utility/strings.C \
                \
                utility/system.C \
                utility/system-stackTrace.C \
                \
                utility/sequence.C \
                \
                utility/types.C \
                \
                utility/kmers-exact.C \
                utility/kmers-files.C \
                utility/kmers-histogram.C \
                utility/kmers-reader.C \
                utility/kmers-writer-block.C \
                utility/kmers-writer-stream.C \
                utility/kmers-writer.C \
                utility/kmers.C \
                \
                utility/bits.C \
                \
                utility/hexDump.C \
                utility/md5.C \
                utility/mt19937ar.C \
                utility/speedCounter.C \
                utility/sweatShop.C \
                \
                parasail/cpuid.c \
                parasail/memory.c \
                parasail/memory_sse.c \
                parasail/memory_avx2.c \
                parasail/sg.c \
                parasail/sg_trace.c \
                parasail/sg_trace_striped_sse2_128_16.c \
                parasail/sg_trace_striped_sse2_128_32.c \
                parasail/sg_trace_striped_sse41_128_16.c \
                parasail/sg_trace_striped_sse41_128_32.c \
                parasail/sg_trace_striped_avx2_256_16.c \
                parasail/sg_trace_striped_avx2_256_32.c \
                parasail/sg_qx_dispatch.c \
                parasail/sg_qb_de_dispatch.c \
                parasail/sg_qe_db_dispatch.c \
                parasail/cigar.c


ifeq (${BUILDSTACKTRACE}, 1)
SOURCES      += utility/libbacktrace/atomic.c \
                utility/libbacktrace/backtrace.c \
                utility/libbacktrace/dwarf.c \
                utility/libbacktrace/elf.c \
                utility/libbacktrace/fileline.c \
                utility/libbacktrace/mmap.c \
                utility/libbacktrace/mmapio.c \
                utility/libbacktrace/posix.c \
                utility/libbacktrace/print.c \
                utility/libbacktrace/simple.c \
                utility/libbacktrace/sort.c \
                utility/libbacktrace/state.c \
                utility/libbacktrace/unknown.c
endif



SRC_INCDIRS  := . \
                utility \
                parasail

SUBMAKEFILES := 

ifeq ($(BUILDTESTS), 1)
SUBMAKEFILES += tests/alignTest-ssw.mk \
                tests/alignTest-ksw2.mk \
                tests/bitsTest.mk \
                tests/filesTest.mk \
                tests/intervalListTest.mk \
                tests/loggingTest.mk \
                tests/parasailTest.mk \
                tests/stddevTest.mk \
                tests/systemTest.mk
endif
