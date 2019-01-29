
#  If 'make' isn't run from the root directory, we need to set these to
#  point to the upper level build directory.

ifeq "$(strip ${DESTDIR})" ""
  DESTDIR      := 
endif

ifeq "$(strip ${PREFIX})" ""
  ifeq "$(strip ${DESTDIR})" ""
    PREFIX     := $(realpath ..)
  else
    PREFIX     := /canu
  endif
endif

ifeq "$(strip ${BUILD_DIR})" ""
  BUILD_DIR    := $(DESTDIR)$(PREFIX)/$(OSTYPE)-$(MACHINETYPE)/obj
endif

ifeq "$(strip ${TARGET_DIR})" ""
  TARGET_DIR   := $(DESTDIR)$(PREFIX)/$(OSTYPE)-$(MACHINETYPE)
endif

TARGET       := libcanu.a

SOURCES      := AS_global.C \
                \
                utility/files.C \
                utility/files-buffered.C \
                utility/files-compressed.C \
                utility/files-memoryMapped.C \
                \
                utility/strings.C \
                \
                utility/system.C \
                utility/system-stackTrace.C \
                \
                utility/sequence.C \
                \
                utility/kmers.C \
                utility/kmers-reader.C \
                utility/kmers-writer.C \
                utility/kmers-writer-block.C \
                utility/kmers-writer-stream.C \
                utility/kmers-statistics.C \
                utility/kmers-exact.C \
                \
                utility/bits.C \
                \
                utility/hexDump.C \
                utility/md5.C \
                utility/mt19937ar.C \
                utility/objectStore.C \
                utility/speedCounter.C \
                utility/sweatShop.C \
                \
                correction/computeGlobalScore.C \
                correction/falconConsensus.C \
                correction/falconConsensus-alignTag.C \
                \
                stores/sqLibrary.C \
                stores/sqStore.C \
                stores/sqStoreConstructor.C \
                stores/sqStoreInfo.C \
                stores/sqStoreEncode.C \
                stores/sqStorePartition.C \
                \
                stores/ovOverlap.C \
                stores/ovStore.C \
                stores/ovStoreWriter.C \
                stores/ovStoreFilter.C \
                stores/ovStoreFile.C \
                stores/ovStoreHistogram.C \
                \
                stores/tgStore.C \
                stores/tgTig.C \
                stores/tgTigSizeAnalysis.C \
                stores/tgTigMultiAlignDisplay.C \
                \
                stores/libsnappy/snappy-sinksource.cc \
                stores/libsnappy/snappy-stubs-internal.cc \
                stores/libsnappy/snappy.cc \
                \
                overlapInCore/overlapReadCache.C \
                \
                overlapErrorAdjustment/analyzeAlignment.C \
                \
                overlapInCore/liboverlap/Binomial_Bound.C \
                overlapInCore/liboverlap/Display_Alignment.C \
                overlapInCore/liboverlap/prefixEditDistance.C \
                overlapInCore/liboverlap/prefixEditDistance-allocateMoreSpace.C \
                overlapInCore/liboverlap/prefixEditDistance-extend.C \
                overlapInCore/liboverlap/prefixEditDistance-forward.C \
                overlapInCore/liboverlap/prefixEditDistance-reverse.C \
                \
                overlapInCore/libedlib/edlib.C \
                \
                utgcns/libNDalign/NDalign.C \
                \
                utgcns/libNDalign/Binomial_Bound.C \
                utgcns/libNDalign/NDalgorithm.C \
                utgcns/libNDalign/NDalgorithm-allocateMoreSpace.C \
                utgcns/libNDalign/NDalgorithm-extend.C \
                utgcns/libNDalign/NDalgorithm-forward.C \
                utgcns/libNDalign/NDalgorithm-reverse.C \
                \
                utgcns/libpbutgcns/AlnGraphBoost.C  \
                \
                gfa/gfa.C \
                gfa/bed.C


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
                stores \
                stores/libsnappy \
                alignment \
                utgcns/libNDalign \
                utgcns/libcns \
                utgcns/libpbutgcns \
                utgcns/libNDFalcon \
                utgcns/libboost \
                overlapInCore \
                overlapInCore/libedlib \
                overlapInCore/liboverlap

SUBMAKEFILES := stores/dumpBlob.mk \
                stores/ovStoreBuild.mk \
                stores/ovStoreConfig.mk \
                stores/ovStoreBucketizer.mk \
                stores/ovStoreSorter.mk \
                stores/ovStoreIndexer.mk \
                stores/ovStoreDump.mk \
                stores/ovStoreStats.mk \
                stores/sqStoreCreate.mk \
                stores/sqStoreCreatePartition.mk \
                stores/sqStoreDumpFASTQ.mk \
                stores/sqStoreDumpMetaData.mk \
                stores/tgStoreCompress.mk \
                stores/tgStoreDump.mk \
                stores/tgStoreLoad.mk \
                stores/tgStoreFilter.mk \
                stores/tgStoreCoverageStat.mk \
                stores/tgTigDisplay.mk \
                stores/loadCorrectedReads.mk \
                stores/loadTrimmedReads.mk \
                stores/loadErates.mk \
                \
                meryl/meryl.mk \
                meryl/meryl-import.mk \
                meryl/meryl-lookup.mk \
                \
                sequence/sequence.mk \
                \
                overlapInCore/overlapInCore.mk \
                overlapInCore/overlapInCorePartition.mk \
                overlapInCore/overlapConvert.mk \
                overlapInCore/overlapImport.mk \
                overlapInCore/overlapPair.mk \
                overlapInCore/edalign.mk \
                \
                overlapInCore/liboverlap/prefixEditDistance-matchLimitGenerate.mk \
                \
                mhap/mhapConvert.mk \
                \
                minimap/mmapConvert.mk \
                \
                wtdbg/wtdbgConvert.mk \
                \
                correction/filterCorrectionOverlaps.mk \
                correction/generateCorrectionLayouts.mk \
                correction/filterCorrectionLayouts.mk \
                correction/falconsense.mk \
                \
                correction/readConsensus.mk \
                correction/errorEstimate.mk \
                \
                haplotyping/splitHaplotype.mk \
                \
                overlapBasedTrimming/trimReads.mk \
                overlapBasedTrimming/splitReads.mk \
                \
                overlapErrorAdjustment/findErrors.mk \
                overlapErrorAdjustment/findErrors-Dump.mk \
                overlapErrorAdjustment/correctOverlaps.mk \
                \
                bogart/bogart.mk \
                \
                bogus/bogus.mk \
                \
                utgcns/utgcns.mk \
                \
                gfa/alignGFA.mk \
                \
                fastq-utilities/fastqAnalyze.mk \
                fastq-utilities/fastqSample.mk \
                fastq-utilities/fastqSimulate.mk \
                fastq-utilities/fastqSimulate-sort.mk

ifeq ($(BUILDTESTS), 1)
SUBMAKEFILES += utility/bitsTest.mk \
                utility/filesTest.mk \
                utility/stddevTest.mk
endif
