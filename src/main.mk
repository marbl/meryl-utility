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
                datastructures/regex-v1.C \
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
                system/cpuIdent-v1.C \
                system/logging-v1.C \
                system/runtime-v1.C \
                system/speedCounter-v1.C \
                system/sweatShop-v1.C \
                system/system-stackTrace-v1.C \
                system/system-v1.C \
                system/time-v1.C \
                \
                sequence/dnaSeq-v1.C \
                sequence/dnaSeqFile-v1.C \
                sequence/sequence-v1.C \
                \
                htslib/hts/bcf_sr_sort.c \
                htslib/hts/bgzf.c \
                htslib/hts/errmod.c \
                htslib/hts/faidx.c \
                htslib/hts/header.c \
                htslib/hts/hfile.c \
                htslib/hts/hts.c \
                htslib/hts/hts_expr.c \
                htslib/hts/hts_os.c \
                htslib/hts/kfunc.c \
                htslib/hts/kstring.c \
                htslib/hts/md5.c \
                htslib/hts/multipart.c \
                htslib/hts/probaln.c \
                htslib/hts/realn.c \
                htslib/hts/regidx.c \
                htslib/hts/region.c \
                htslib/hts/sam.c \
                htslib/hts/synced_bcf_reader.c \
                htslib/hts/tbx.c \
                htslib/hts/textutils.c \
                htslib/hts/thread_pool.c \
                htslib/hts/vcf.c \
                htslib/hts/vcf_sweep.c \
                htslib/hts/vcfutils.c \
                htslib/cram/cram_codecs.c \
                htslib/cram/cram_decode.c \
                htslib/cram/cram_encode.c \
                htslib/cram/cram_external.c \
                htslib/cram/cram_index.c \
                htslib/cram/cram_io.c \
                htslib/cram/cram_stats.c \
                htslib/cram/mFILE.c \
                htslib/cram/open_trace_file.c \
                htslib/cram/pooled_alloc.c \
                htslib/cram/string_alloc.c \
                htslib/htscodecs/arith_dynamic.c \
                htslib/htscodecs/fqzcomp_qual.c \
                htslib/htscodecs/htscodecs.c \
                htslib/htscodecs/pack.c \
                htslib/htscodecs/rANS_static.c \
                htslib/htscodecs/rANS_static32x16pr.c \
                htslib/htscodecs/rANS_static4x16pr.c \
                htslib/htscodecs/rle.c \
                htslib/htscodecs/tokenise_name3.c \
                htslib/htscodecs/utils.c \
                \
                parasail/cpuid.c \
                parasail/memory.c \
                parasail/sg.c \
                parasail/sg_trace.c \
                parasail/sg_qx_dispatch.c \
                parasail/sg_qb_de_dispatch.c \
                parasail/sg_qe_db_dispatch.c \
                parasail/cigar.c

#                htslib/htscodecs/rANS_static32x16pr_avx2.c \
#                htslib/htscodecs/rANS_static32x16pr_avx512.c \
#                htslib/htscodecs/rANS_static32x16pr_neon.c \
#                htslib/htscodecs/rANS_static32x16pr_sse4.c \

ifeq (${BUILDSTACKTRACE}, 1)
SOURCES      += system/libbacktrace/atomic.c \
                system/libbacktrace/backtrace.c \
                system/libbacktrace/dwarf.c \
                system/libbacktrace/elf.c \
                system/libbacktrace/fileline.c \
                system/libbacktrace/mmap.c \
                system/libbacktrace/mmapio.c \
                system/libbacktrace/posix.c \
                system/libbacktrace/print.c \
                system/libbacktrace/simple.c \
                system/libbacktrace/sort.c \
                system/libbacktrace/state.c \
                system/libbacktrace/unknown.c
endif



SRC_INCDIRS  := .

SUBMAKEFILES := pccp/pccp.mk \
                regex/regex.mk

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
                tests/count-palindromes.mk \
                tests/loggingTest.mk \
                tests/magicNumber.mk \
                tests/parasailTest.mk \
                tests/testVectorSupport.mk \
                tests/readLines.mk \
                tests/regexTest.mk \
                tests/sequenceTest.mk \
                tests/stddevTest.mk \
                tests/stringsTest.mk \
                tests/systemTest.mk \
                tests/testHTSbam.mk \
                tests/testHTSlib.mk \
                tests/timeTest.mk \
                tests/toHexTest.mk \
                tests/typesTest.mk
endif
