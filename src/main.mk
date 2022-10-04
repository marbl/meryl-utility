MODULE       :=    meryl-utility
TARGET       := libmeryl-utility.a
SOURCES      := \
                \
                align/align-ksw2-driver.C \
                align/align-ksw2-extz.C \
                align/align-ksw2-extz2-sse.C \
                align/align-parasail-driver.C \
                align/align-ssw-driver.C \
                align/align-ssw.C \
                align/edlib.C \
                \
                bits/fibonacci-v1.C \
                bits/hexDump-v1.C \
                bits/stuffedBits-v1.C \
                bits/stuffedBits-v1-binary.C \
                bits/stuffedBits-v1-bits.C \
                bits/stuffedBits-v1-delta.C \
                bits/stuffedBits-v1-gamma.C \
                bits/stuffedBits-v1-golomb.C \
                bits/stuffedBits-v1-omega.C \
                bits/stuffedBits-v1-unary.C \
                bits/stuffedBits-v1-zeckendorf.C \
                bits/wordArray-v1.C \
                \
                datastructures/strings-v1.C \
                datastructures/keyAndValue-v1.C \
                datastructures/splitToWords-v1.C \
                datastructures/stringList-v1.C \
                datastructures/types-v1.C \
                \
                files/accessing-v1.C \
                files/buffered-v1-reading.C \
                files/buffered-v1-writing.C \
                files/compressed-v1.C \
                files/compressed-v1-reading.C \
                files/compressed-v1-writing.C \
                files/fasta-fastq-v1.C \
                files/memoryMapped-v1.C \
                files/reading-v1.C \
                files/files-v1.C \
                files/writing-v1.C \
                files/readLine-v0.C \
                files/readLine-v1.C \
                \
                kmers-v1/kmers-exact.C \
                kmers-v1/kmers-files.C \
                kmers-v1/kmers-histogram.C \
                kmers-v1/kmers-reader.C \
                kmers-v1/kmers-writer-block.C \
                kmers-v1/kmers-writer-stream.C \
                kmers-v1/kmers-writer.C \
                kmers-v1/kmers.C \
                \
                kmers-v2/kmers-exact.C \
                kmers-v2/kmers-files.C \
                kmers-v2/kmers-histogram.C \
                kmers-v2/kmers-reader-dump.C \
                kmers-v2/kmers-reader.C \
                kmers-v2/kmers-writer-block.C \
                kmers-v2/kmers-writer-stream.C \
                kmers-v2/kmers-writer.C \
                kmers-v2/kmers.C \
                \
                math/md5-v1.C \
                math/mt19937ar-v1.C \
                math/sampledDistribution-v1.C \
                \
                system/logging-v1.C \
                system/runtime-v1.C \
                system/speedCounter-v1.C \
                system/sweatShop-v1.C \
                system/system-stackTrace-v1.C \
                system/system-v1.C \
                \
                sequence/dnaSeq-v1.C \
                sequence/dnaSeqFile-v1.C \
                sequence/sequence-v1.C \
                \
                \
                \
                parasail/cpuid.c \
                parasail/memory.c \
                parasail/sg.c \
                parasail/sg_trace.c \
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
                parasail

SUBMAKEFILES := pccp/pccp.mk

ifeq ($(BUILDTESTS), 1)
SUBMAKEFILES += tests/alignTest-ssw.mk \
                tests/alignTest-ksw2.mk \
                tests/bitsTest.mk \
                tests/commandAvailableTest.mk \
                tests/decodeIntegerTest.mk \
                tests/fasta-fastq.mk \
                tests/filesTest.mk \
                tests/intervalListTest.mk \
                tests/intervalsTest.mk \
                tests/loggingTest.mk \
                tests/magicNumber.mk \
                tests/parasailTest.mk \
                tests/readLines.mk \
                tests/sequenceTest.mk \
                tests/stddevTest.mk \
                tests/stringsTest.mk \
                tests/systemTest.mk \
                tests/toHexTest.mk \
                tests/typesTest.mk
endif
