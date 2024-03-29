
/******************************************************************************
 *
 *  This file is part of meryl-utility, a collection of miscellaneous code
 *  used by Meryl, Canu and others.
 *
 *  This software is based on:
 *    'Canu' v2.0              (https://github.com/marbl/canu)
 *  which is based on:
 *    'Celera Assembler' r4587 (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' r1994 (http://kmer.sourceforge.net)
 *
 *  Except as indicated otherwise, this is a 'United States Government Work',
 *  and is released in the public domain.
 *
 *  File 'README.licenses' in the root directory of this distribution
 *  contains full conditions and disclaimers.
 */

#include <vector>
#include <algorithm>

#include "kmers.H"

//  Some sanity checking.
#undef  TEST_MAXP
#undef  CHECK_POINTERS
#undef  CHECK_STORE

namespace merylutil::inline kmers::v1 {


//  Set some basic boring stuff.
//
void
merylExactLookup::initialize(merylFileReader *input_,
                             kmvalu minValue_,
                             kmvalu maxValue_) {

  //  Save a pointer to the input data.

  _input = input_;

  //  Initialize filtering, silently make minValue and maxValue be valid values.

  if (minValue_ == 0)           minValue_ = 1;
  if (maxValue_ == kmvalumax)   maxValue_ = _input->stats()->maxValue();

  _minValue       = minValue_;
  _maxValue       = maxValue_;

  //  Now initialize table parameters!  (all the rest are in the declaration initializer)

  _Kbits = kmer::merSize() * 2;

  //  If we're storing values too, compute some parameters.
  //  _valueOffset lets us store a '1' in the table to represent 'minValue'.
  //
  if (_maxValue >= _minValue) {
    _valueOffset = minValue_ - 1;
    _valueBits   = countNumberOfBits64(_maxValue + 1 - _minValue);
    _valueMask   = buildLowBitMask<kmvalu>(_valueBits);
  }

  //  Scan the histogram to count the number of kmers in range.

  _nSuffix = 0;

  for (uint32 ii=0; ii<_input->stats()->histogramLength(); ii++) {
    kmvalu  v = _input->stats()->histogramValue(ii);

    if ((_minValue <= v) &&
        (v <= _maxValue))
      _nSuffix += _input->stats()->histogramOccurrences(ii);
  }
}


void
merylExactLookup::computeSpace(bool reportMemory,
                               bool reportSizes,
                               bool compute,
                               uint32 &pbMin, uint64 &minSpace) {
  uint32  pbbgn;
  uint32  pbend;

  //  If not computing, show values four before and four after the supplied
  //  prefix size.

  if (compute == false) {
    pbbgn = pbMin - 4;
    pbend = pbMin + 5;

    if (pbbgn < 6)        pbbgn = 6;          //  Fix some silly cases.
    if (pbend > _Kbits)   pbend = _Kbits;
  }

  //  If computing, iterate prefix sizes from 6 until the largest useful,
  //  unless there is a suggested prefix size, then compute exactly and only
  //  that.

  else if (pbMin == 0) {
    pbbgn = 6;
    pbend = countNumberOfBits64(_nSuffix + 64 * 256) + 1;

    if (pbend > kmer::merSize() * 2)
      pbend = kmer::merSize() * 2;
  }
  else {
    pbbgn = pbMin;
    pbend = pbMin + 1;
  }

  //  Write a header if reporting.

  if (reportMemory) {
    fprintf(stderr, "\n");
    fprintf(stderr, " p         blocks   ent/blk             bits gigabytes (allowed: %lu GB)\n", _maxMemory >> 33);
    fprintf(stderr, "-- -------------- --------- ---------------- ---------\n");
  }

  //  Do the compute.
  //
  //  A significant problem here is that we CANNOT know 'blocklw' without
  //  knowing the kmers.  Are all kmers in the same block?  Are they
  //  uniformly spread throughout?  The best we could do is infer this from
  //  the size of each file in the database.

  char   summary[7][128] = { {0}, {0}, {0}, {0}, {0}, {0}, {0} };

  for (uint32 pb=pbbgn; pb<pbend; pb++) {
    uint64  pointerw = countNumberOfBits64(_nSuffix + 64 * 256);   //  Width of a block pointer, log2(#kmers)
    uint64  blocklw;                                               //  Width of block length, computed below.
    uint64  tagw     = _Kbits - pb;                                //  Width of kmer suffix data
    uint64  valuw    = _valueBits;                                 //  Width of kmer value

    uint64  nprefix  = uint64one << pb;    //  Number of block pointers
    uint64  nsuffix  = _nSuffix;           //  Number of entries kmer suffix array
    uint64  nthreads = getNumThreads();    //  Number of threads that will be loading data.

#warning This is a complete guess.
    blocklw = countNumberOfBits64(16 * _nSuffix / nprefix);

    uint64  space    = (nprefix *  pointerw          +   //  Space for pointers to blocks
                        nprefix *  blocklw           +   //  Space for lengths of blocks
                        nsuffix *  tagw              +   //  Space for kmer tags
                        nsuffix * _valueBits         +   //  Space for kmer values
                        nprefix * sizeof(uint32) * 8 +   //  Space for temporary block length
                        nthreads * 512 * 1048576 * 8);   //  Space for load buffers

    //  If we're told to compute values, save values that have the smallest
    //  and 'optimal' space usage.

    if ((compute) && (space < minSpace)) {
      pbMin    = pb;
      minSpace = space;
    }

    //  If reporting, report!

    if (reportMemory) {
      fprintf(stderr, "%2u %14lu %9lu %16lu %9.3f%s\n", pb, nprefix, 0lu, space, bitsToGB(space),
              (pb == pbMin) ? "  (smallest)" : "");
    }

    if ((reportSizes) && (pb == pbMin)) {
      snprintf(summary[0], 128, "For %lu distinct %u-mers (with %u bits used for indexing and %lu bits for tags):\n", nsuffix, _Kbits / 2, pb, tagw);
      snprintf(summary[1], 128, "  %7.3f GB memory for block indices - %12lu elements %2lu bits wide)\n", bitsToGB(nprefix *  pointerw), nprefix, pointerw);
      snprintf(summary[2], 128, "  %7.3f GB memory for block lengths - %12lu elements %2lu bits wide)\n", bitsToGB(nprefix *  blocklw),  nprefix, blocklw);
      snprintf(summary[3], 128, "  %7.3f GB memory for kmer tags     - %12lu elements %2lu bits wide)\n", bitsToGB(nsuffix *  tagw),     nsuffix, tagw);
      snprintf(summary[4], 128, "  %7.3f GB memory for kmer values   - %12lu elements %2lu bits wide)\n", bitsToGB(nsuffix *  valuw),    nsuffix, valuw);
      snprintf(summary[5], 128, "  %7.3f GB memory for buffers\n", bitsToGB(nprefix * sizeof(uint32) * 8 + nthreads * 512 * 1048576 * 8));
      snprintf(summary[6], 128, "  %7.3f GB memory\n", bitsToGB(space));
    }
  }

  //  Write a footer and summary if reporting.

  if (reportMemory) {
    fprintf(stderr, "-- -------------- --------- ---------------- ---------\n");
    fprintf(stderr, "   %14lu total kmers\n", _nSuffix);
  }

  if (reportSizes) {
    fprintf(stderr, "\n");
    fprintf(stderr, "%s", summary[0]);
    fprintf(stderr, "%s", summary[1]);
    fprintf(stderr, "%s", summary[2]);
    fprintf(stderr, "%s", summary[3]);
    fprintf(stderr, "%s", summary[4]);
    fprintf(stderr, "%s", summary[5]);
    fprintf(stderr, "%s", summary[6]);
  }
}


//  Analyze the number of kmers to store in the table, to decide on
//  various parameters for allocating the table - how many bits to
//  use for indexing (prefixSize), and how many bits of data we need
//  to store explicitly (suffixBits and valueBits).
//
double
merylExactLookup::configure(double  memInGB,
                            uint32  prefixSize,
                            bool    reportMemory,
                            bool    reportSizes) {

  //  Convert the memory in GB to memory in BITS.  If no memory
  //  size is supplied, as the OS how big we can get.

  if (memInGB == 0.0)
    _maxMemory = getMaxMemoryAllowed() * 8;
  else
    _maxMemory = (uint64)(memInGB * 1024.0 * 1024.0 * 1024.0 * 8);

  //  Find the prefixBits that results in the smallest allocated memory size.
  //  Due to threading over the files, we cannot use a prefix smaller than 6
  //  bits.
  //
  //  Note that _nSuffix is the post-value-filtered number of kmers we
  //  will load into our table.
  //
  //  There is a BIG problem in this method in that we DO NOT know how
  //  many kmers will be stored in each bucket without knowing the kmers
  //  themselves.

  uint32  pbMin      = prefixSize;
  uint64  minSpace   = uint64max;

  computeSpace(false, false, true, pbMin, minSpace);

  //  Set parameters.

  _prefixBits  = pbMin;
  _suffixBits  = _Kbits - _prefixBits;
  _suffixMask  = buildLowBitMask<kmdata>(_suffixBits);
  _nPrefix     = uint64one << _prefixBits;

  //  Compute again, this time just for logging.

  computeSpace(reportMemory, reportSizes, false, pbMin, minSpace);

  //  Return the memory required.

  return(bitsToGB(minSpace));
}



void
merylExactLookup::setPointers(uint64 ii, uint64 bgn, uint32 len) {
  uint64  b, e;

  bool    l = (_verbose == true) && ((ii & 0xfffffff) == 0xfffffff);   //  Logging enabled?
  bool    c = false;                                                   //  Checking enabled?

  _sufPointer->set(ii, (bgn << _blkLenBits) | len);

  if ((l == false) && (c == false))
    return;

  getPointers(ii, b, e);

  if ((b != bgn) ||
      (e != bgn + len))
    fprintf(stderr, "merylExactLookup::setPointers()- FAIL bgn=%lu len=%u != b=%lu e=%lu\n", bgn, len, b, e);

  assert(b == bgn);
  assert(e == bgn + len);

  if (l) {
    fprintf(stderr, " %11lu [%06u] -> %11lu [%06lu]    progress: %11lu / %-11lu\r",
            bgn, len, b, e-b, ii, _nPrefix);

    if (ii + 0xfffffff > _nPrefix)   //  If this is the last progress report,
      fprintf(stderr, "\n");         //  advance the line.
  }
}



//  Make one pass through the file to count how many kmers per prefix we will end
//  up with.  This is needed only if kmers are filtered, but does
//  make the rest of the loading a little easier.
//
//  The loop control and kmer loading is the same in the two loops.
void
merylExactLookup::count(void) {
  uint32   nf = _input->numFiles();

  if (_verbose) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Counting size of buckets.\n");
  }

  uint32  *blockLength = new uint32 [_nPrefix];        //  Temporary array holding the length of
  memset(blockLength, 0, sizeof(uint32) * _nPrefix);   //  each block in our data.

  merylExactLookupProgress   progress(" Counting: [%s]  (* - complete)\r");

  //  If the min/max prefix for each input file intersect, we've got a
  //  problem somewhere.  Each 'prefix' will map to exactly one file, and
  //  they're supposed to map consecutively.  Good luck figuring out what
  //  broke if this triggers.  This will fail with prefixBits < 6 (see also
  //  line ~150) as we don't have enough different prefixes to assigned them
  //  to different files.
#ifdef TEST_MAXP
  uint64   minp[2 * nf];

  for (uint32 ii=0; ii<nf; ii++) {
    minp[2*ii+0] = uint64max;
    maxp[2*ii+1] = uint64min;
  }
#endif

  //  Scan all kmer files, counting the number of kmers per prefix.
  //  This is thread safe when _prefixBits is more than 6 (the number of files).
#pragma omp parallel for schedule(dynamic, 1)
  for (uint32 ff=0; ff<nf; ff++) {
    FILE                  *blockFile = _input->blockFile(ff);
    merylFileBlockReader  *block     = new merylFileBlockReader;
    uint64                 tooLow  = 0;    //  Local counters, otherwise threads collide.
    uint64                 tooHigh = 0;
    uint64                 loaded  = 0;

    //  Load blocks until there are no more.

    while (block->loadBlock(blockFile, ff) == true) {
      progress.tick(ff);
      progress.show();

      block->decodeBlock();
      progress.tick(ff);
      progress.show();

      for (uint32 ss=0; ss<block->nKmers(); ss++) {
        kmdata   kbits  = 0;                     //  The reconstructed kmer.
        kmdata   prefix = 0;                     //  prefix bits of that kmer.
        kmvalu   value  = block->values()[ss];   //  Value of the kmer, used to filter.

        if ((ss & 0x7fff) == 0) {
          progress.tick(ff);
          progress.show();
        }

        if (value < _minValue)   { tooLow++;   continue; }
        if (value > _maxValue)   { tooHigh++;  continue; }

        loaded++;

        kbits   = block->prefix();         //  Combine the file prefix and
        kbits <<= _input->suffixSize();    //  suffix data to reconstruct
        kbits  |= block->suffixes()[ss];   //  the kmer bits.

        prefix = kbits >> _suffixBits;     //  Then extract the prefix

#ifdef TEST_MAXP
        minp[ff] = std::min(minp[ff], (uint64)prefix);
        maxp[ff] = std::max(maxp[ff], (uint64)prefix);
#endif

        assert(prefix < _nPrefix);

        blockLength[prefix]++;                  //  Count the number of kmers per prefix.
      }
    }

#pragma omp critical (count_stats)
    {
      _nKmersTooLow  += tooLow;
      _nKmersTooHigh += tooHigh;
      _nKmersLoaded  += loaded;
    }

    delete block;

    closeFile(blockFile);

    progress.stop(ff);
  }

#ifdef TEST_MAXP
  for (uint32 ii=1; ii<nf; ii++)
    assert(maxp[ii-1] < minp[ii]);
#endif

  //  Build an array (bgn,len) - the begin location and length of data - for
  //  each kmer prefix
  //
  //  While wordArray does have locks to prevent multiple threads from writing
  //  to the same block, they're a bit slow here, especially given that we know
  //  ahead of time where the collisions will occur.  To prevent collisions,
  //  we can insert some empty values between each block.
  //
  //  A single thread will process all kmers [ffffffpppppppp|ssssssss] that
  //  have the same ffffff bits.              --- prefix ---|-suffix-
  //
  //  The ffffff bits are the file id bits, currently (and almost certainly
  //  always) 6 bits wide.  'prefix' is _prefixBits wide; 'suffix' is
  //  _suffixBits wide.  The end of a block occurs when all the p bits are 1,
  //  so when we encounter a prefix with those set, we advance the begin
  //  pointer by a goodly amount to prevent thread A from writing to
  //  ffffff1111 and another from writing to the next word (ffffff+1)0000.

  if (_verbose) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Summing bucket sizes.\n");
  }

  uint64 bgnp = 0;
  uint64 maxp = 63 * 256;   //  Maximum index, including the adjustments.
  uint64 maxl = 0;          //  Maximum length of any block.

  uint64 amask = buildLowBitMask<uint64>(_prefixBits - 6);  //  Mask covering the p bits of prefix.

  for (uint64 ii=0; ii<_nPrefix; ii++) {
    maxp +=          (uint64)blockLength[ii];
    maxl  = std::max((uint64)blockLength[ii], maxl);
  }

  _blkPtrBits = countNumberOfBits64(maxp);   _blkPtrMask = buildLowBitMask<uint64>(_blkPtrBits);
  _blkLenBits = countNumberOfBits64(maxl);   _blkLenMask = buildLowBitMask<uint64>(_blkLenBits);

  fprintf(stderr, "  block indices are %u bits wide -- sum lengths %lu (including %d empty pointers)\n", _blkPtrBits, maxp, 63 * 256);
  fprintf(stderr, "  block lengths are %u bits wide -- max length  %lu\n", _blkLenBits, maxl);

  //  Allocate a wordArray (disabling locking) to store the pointers to the
  //  data block for each prefix.  Then compute the start of each block and
  //  store it and the length of the block in the array.

  fprintf(stderr, "\n");
  fprintf(stderr, "Setting pointers.\n");

  _sufPointer = new wordArray(_blkPtrBits + _blkLenBits, 32llu * 1024 * 1024 * 8, false);
  _sufPointer->allocate(maxp + 1);

  for (uint64 ii=0; ii<_nPrefix; ii++) {
    setPointers(ii, bgnp, blockLength[ii]);

    bgnp += blockLength[ii];
    assert(bgnp <= maxp);

    if ((ii & amask) == amask)   //  Move ahead a goodly amount so we don't collide
      bgnp += 256;               //  on thread boundaries.
  }

  assert(bgnp - 256 == maxp);

  _nIndex = maxp;

  delete [] blockLength;

  //  Log.

  fprintf(stderr, "\n");
  fprintf(stderr, "Will load " F_U64 " kmers.  Skipping " F_U64 " (too low) and " F_U64 " (too high) kmers.\n",
          _nKmersLoaded, _nKmersTooLow, _nKmersTooHigh);
}



//  With all parameters known, just grab and clear memory.
//
//  The block size used in the wordArray _sufData is chosen so that large
//  arrays have not-that-many allocations.  The array is pre-allocated, to
//  prevent the need for any locking or coordination when filling out the
//  array.
//
double
merylExactLookup::allocate(void) {
  uint64  arraySize;
  uint64  arrayBlockMin;
  double  memInGBused = 0.0;

  if (_verbose) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Allocating space for %lu kmer positions.\n", _nIndex);
  }

  if (_suffixBits > 0) {
    arraySize      = _nIndex * _suffixBits;
    arrayBlockMin  = std::max(arraySize / 1024llu, 32llu * 1024 * 1024 * 8);   //  In bits, 32MB per block.
    memInGBused   += bitsToGB(arraySize);

    if (_verbose) {
      fprintf(stderr, "  suffixes of %3u bits each -> %12lu bits (%7.3f GB) in blocks of %7.3f MB\n",
              _suffixBits, arraySize, bitsToGB(arraySize), bitsToMB(arrayBlockMin));
    }
    assert(_suffixBits <= 128);

    _sufData = new wordArray(_suffixBits, arrayBlockMin, false);
    _sufData->allocate(_nIndex);
    _sufData->erase(0, _nIndex);
  }

  if (_valueBits > 0) {
    arraySize      = _nIndex * _valueBits;
    arrayBlockMin  = std::max(arraySize / 1024llu, 32llu * 1024 * 1024 * 8);   //  In bits, 32MB per block.
    memInGBused   += bitsToGB(arraySize);

    if (_verbose) {
      fprintf(stderr, "  values   of %3u bits each -> %12lu bits (%7.3f GB) in blocks of %7.3f MB\n",
              _valueBits,  arraySize, bitsToGB(arraySize), bitsToMB(arrayBlockMin));
    }
    assert(_valueBits <= 64);

    _valData = new wordArray(_valueBits, arrayBlockMin, false);
    _valData->allocate(_nIndex);
    _valData->erase(0, _nIndex);
  }

  return(memInGBused);
}



//  Each file can be processed independently IF we know how many kmers are in
//  each prefix.  For that, we need to load the merylFileReader index.
//  We don't, actually, know that if we're filtering out low/high count kmers.
//  In this case, we overallocate, but cannot cleanup at the end.
void
merylExactLookup::load(void) {
  uint32   nf      = _input->numFiles();

  if (_verbose) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Filling buckets.\n");
  }

  assert(buildLowBitMask<kmvalu>(_valueBits)  == _valueMask);
  assert(buildLowBitMask<kmdata>(_suffixBits) == _suffixMask);

  uint32  *prefixCount = new uint32 [_nPrefix];        //  Temporary array holding the length of
  memset(prefixCount, 0, sizeof(uint32) * _nPrefix);   //  each block.

  merylExactLookupProgress   progress(" Loading:  [%s]  (* - complete)\r");

#pragma omp parallel for schedule(dynamic, 1)
  for (uint32 ff=0; ff<nf; ff++) {
    FILE                  *blockFile = _input->blockFile(ff);
    merylFileBlockReader  *block     = new merylFileBlockReader;

    //  Load blocks until there are no more.

    while (block->loadBlock(blockFile, ff) == true) {
      progress.tick(ff);
      progress.show();

      block->decodeBlock();
      progress.tick(ff);
      progress.show();

      for (uint32 ss=0; ss<block->nKmers(); ss++) {
        kmdata   kbits  = 0;
        kmdata   prefix = 0;
        kmdata   suffix = 0;
        kmvalu   value  = block->values()[ss];
        uint64   bgn, end, loc;

        if ((ss & 0x7fff) == 0) {
          progress.tick(ff);
          progress.show();
        }

        if ((value < _minValue) ||         //  Sanity checking and counting done
            (_maxValue < value))           //  in count() above.
          continue;

        kbits   = block->prefix();         //  Combine the file prefix and
        kbits <<= _input->suffixSize();    //  suffix data to reconstruct
        kbits  |= block->suffixes()[ss];   //  the kmer bits.

        prefix = kbits >> _suffixBits;     //  Extract prefix.
        suffix = kbits  & _suffixMask;     //  Extract suffix.

        getPointers(prefix, bgn,  end);    //  Get the block pointer.
        loc = bgn + prefixCount[prefix];

        _sufData->set(loc, suffix);        //  Store the suffix.

        //  Test that it stored correctly.

#ifdef CHECK_STORE
        {
          uint64 val = _sufData->get(loc);

          if (val != suffix) {
            char ks[65];
            kmer k;   k._mer = kbits;

            fprintf(stdout, "STORE kmer %s [ %s | %s ] at position %lu\n",
                    k.toString(ks),
                    toHex(prefix, _prefixBits),
                    toHex(suffix, _suffixBits), loc);

            fprintf(stderr, "FAIL fetched 0x%s\n", toHex(val));
          }

          assert(val == suffix);
        }
#endif

        //  Compute and store the value, if requested.

        if (_valueBits > 0) {
          value -= _valueOffset;

          if (value > _maxValue + 1 - _minValue)
            fprintf(stderr, "minValue " F_U32 " maxValue " F_U32 " value " F_U32 " bits " F_U32 "\n",
                    _minValue, _maxValue, value, _valueBits);
          assert(value <= _valueMask);

          _valData->set(loc, value);
        }

        //  Move to the next item.

        prefixCount[prefix]++;
      }
    }

    delete block;

    closeFile(blockFile);

    progress.stop(ff);
  }

  delete [] prefixCount;

  //  Now just log.

  if (_verbose) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Loaded " F_U64 " kmers.  Skipped " F_U64 " (too low) and " F_U64 " (too high) kmers.\n",
            _nKmersLoaded, _nKmersTooLow, _nKmersTooHigh);
  }
}



double
merylExactLookup::load(merylFileReader *input_,
                       double           maxMemInGB_,
                       uint32           prefixSize_,
                       kmvalu           minValue_,
                       kmvalu           maxValue_) {

  initialize(input_, minValue_, maxValue_);            //  Initialize ourself.

  double memInGBused = configure(maxMemInGB_,          //  Find parameters.
                                 prefixSize_,
                                 true, true);

  if (_prefixBits == 0) {                              //  Fail if needed.
    fprintf(stderr, "Failed to configure: _prefixBits = 0?\n");
    return(memInGBused);
  }

  count();                                             //  Count kmers/prefix.
  memInGBused = allocate();                            //  Allocate space.
  load();                                              //  Load data.

  return(memInGBused);
}



//  Populates two arrays to hold position data for each kmer.
//    uint64       *positionList;
//    stuffedBits  *positions;
//
//    positionList[i] is the start of the position data in 'positions' for
//    kmer with merylExactLookup index() i.
//
//    There are 'count()' positions for that kmer starting there.
//
//    Each position encodes a sequence index and a position in that sequence.
//
//    For human, there will be approximately 2.1 billion index()es and 3.2 billion
//    positions; seqIdx will need 5 bits and position itself will by 29 bits.
//    This will require
//      64 * 2.1e9 / 8 = 16.8 GB for positionList (half that if we use 32-bit ints)
//      34 * 3.2e9 / 8 = 12.7 GB for position data
//
void
merylExactLookup::loadPositions(dnaSeqFile *seqFile) {

  //  Load the sequences.  This doesn't need to be in memory, but we'd need
  //  to (possibly) make two passes otherwise, one to count the number of
  //  sequences (so we know how many bits to allocate for the sequence index)
  //  and one to process the sequence.

  fprintf(stderr, "Loading sequences from '%s'\n", seqFile->filename());

  std::vector<dnaSeq *>  sequences;

  uint64  nSequences = 0;   //  Number of sequences in the input
  uint64  nPositions = 0;   //  Longest sequence length
  uint64  nPosEntry  = 0;   //  Number of position entries we need to store

  {
    dnaSeq  *seq = new dnaSeq;

    while (seqFile->loadSequence(*seq) == true) {
      sequences.push_back(seq);

      nSequences += 1;
      nPositions  = std::max(nPositions, seq->length());
      nPosEntry  += seq->length();

      seq = new dnaSeq;
    }

    delete seq;
  }

  //  Allocate space for a position index and the actual position data.

  nSequencesBits = countNumberOfBits64(nSequences);
  nPositionsBits = countNumberOfBits64(nPositions);
  nPosEntryBits  = countNumberOfBits64(nPosEntry);

  posEntryIDMask  = buildLowBitMask<uint64>(nSequencesBits);
  posEntryPosMask = buildLowBitMask<uint64>(nPositionsBits);

  fprintf(stderr, "Allocate space:\n");
  fprintf(stderr, "  %12lu %2u-bit wide index pointers.\n", nIndex(), nPosEntryBits);
  fprintf(stderr, "  %12lu %2u-bit + %2u-bit wide locations\n", nPosEntry, nSequencesBits, nPositionsBits);

  _posStart = new wordArray(nPosEntryBits, 131072, false);
  _posData  = new wordArray(nSequencesBits + nPositionsBits, 131072, false);

  //  Set the posStart based on the number of each kmer we have in the database.

  fprintf(stderr, "Create initial index.\n");
  for (uint64 pp=0, ii=0; ii<_nIndex; ii++) {
    _posStart->set(ii, pp);
    assert(_posStart->get(ii) == pp);
    pp += valueAtIndex(ii);
  }

  //  Lookup each kmer in the sequences and add a position entry.

  for (uint32 ii=0; ii<sequences.size(); ii++) {
    dnaSeq *seq = sequences[ii];

    fprintf(stderr, "Scan %9lubp from '%s'\n", seq->length(), seq->ident());

    kmerIterator kiter(seq->bases(), seq->length());

    while (kiter.nextMer()) {
      kmer    cmer  = std::min(kiter.fmer(), kiter.rmer());
      uint64  idx   = index(cmer);

      if ((kiter.position() % 10000000) == 0)
        fprintf(stderr, "  %9lu / %9lu\r", kiter.position(), seq->length());
      if (idx == uint64max)   //  Not a kmer we care about.
        continue;

      //  Add a position entry for the kmer instance.
      uint64 ps = _posStart->get(idx);
      _posStart->set(idx, ps + 1);
      assert(_posStart->get(idx) == ps + 1);

      uint64 pos   = (ii << nPositionsBits) | (kiter.bgnPosition());
      _posData->set(ps, pos);
      assert(pos == _posData->get(ps));
    }

    fprintf(stderr, "  %9lu / %9lu\n", kiter.position(), seq->length());
  }

  //  Reset index.

  fprintf(stderr, "Reset index.\n");
  for (uint64 pp=0, ii=0; ii<_nIndex; ii++) {
    _posStart->set(ii, pp);
    assert(_posStart->get(ii) == pp);
    pp += valueAtIndex(ii);
  }

  //  Test it.
#if 1
  char ks[65];

  for (uint32 ii=0; ii<sequences.size(); ii++) {
    dnaSeq *seq = sequences[ii];

    fprintf(stderr, "Test %9lubp from '%s'\n", seq->length(), seq->ident());

    kmerIterator kiter(seq->bases(), seq->length());

    while (kiter.nextMer()) {
      kmer    cmer  = std::min(kiter.fmer(), kiter.rmer());
      uint64  idx   = index(cmer);

      if ((kiter.position() % 10000000) == 0)
        fprintf(stderr, "  %9lu / %9lu\r", kiter.position(), seq->length());
      if (idx == uint64max)   //  Not a kmer we care about.
        continue;

      uint64  base = _posStart->get(idx);
      uint32  nmax = valueAtIndex(idx);
      uint64  pos  = (ii << nPositionsBits) | (kiter.bgnPosition());

      bool found = false;
      for (uint32 nn=0; nn<nmax; nn++) {
        if (_posData->get(base + nn) == pos)
          found = true;
      }

      if (found == false)
        fprintf(stderr, "kmer at %9lu %s has index %9lu %6u hits, found = %s\n",
                kiter.position(), cmer.toString(ks), idx, nmax, found ? "true" : "false");
    }
  }
#endif

  //  Cleanup.

  for (uint32 ii=0; ii<sequences.size(); ii++)
    delete sequences[ii];
}


bool
merylExactLookup::exists_test(kmer k) {
  kmdata  kmer   = (kmdata)k;
  kmdata  prefix = kmer >> _suffixBits;
  kmdata  suffix = kmer  & _suffixMask;
  kmdata  tag;
  uint64  bgn, mid, end;
  char    kmerString[65];

  fprintf(stderr, "\n");
  fprintf(stderr, "kmer        %s  %s\n", toHex(kmer, 2 * k.merSize()), k.toString(kmerString));
  fprintf(stderr, "suffixBits  %s  %3u bits\n", toHex(_suffixMask, _suffixBits), _suffixBits);
  fprintf(stderr, "prefix      %s  %3u bits\n", toHex(prefix, 2 * k.merSize() - _suffixBits), 2 * k.merSize() - _suffixBits);
  fprintf(stderr, "suffix      %s\n", toHex(suffix, _suffixBits));

  //  Do the normal binary search;if found, return that we found it.

  if (exists(k))
    return(true);

  //  If not found, do the binary search again, reporting what we look at.

  getPointers(prefix, bgn,  end);

  fprintf(stderr, "\n");
  fprintf(stderr, "BINARY SEARCH the bucket %lu-%lu for suffix %s.\n", bgn, end, toHex(suffix, _suffixBits));

  while (bgn + 8 < end) {
    mid = bgn + (end - bgn) / 2;

    tag = _sufData->get(mid);

    fprintf(stderr, "TEST bgn %8lu %8lu %8lu end -- dat %s =?= %s suffix\n",
            bgn, mid, end, toHex(tag), toHex(suffix));

    if (tag == suffix) {
      fprintf(stderr, "FOUND?\n");
      assert(0);
      return(true);
    }

    if (suffix < tag)
      end = mid;

    else
      bgn = mid + 1;
  }

  for (mid=bgn; mid < end; mid++) {
    tag = _sufData->get(mid);

    fprintf(stderr, "ITER bgn %8lu %8lu %8lu end -- dat %s\n",
            bgn, mid, end, toHex(tag));

    if (tag == suffix) {
      fprintf(stderr, "FOUND?\n");
      assert(0);
      return(true);
    }
  }

  //  If still not found, search the whole bucket.

  getPointers(prefix, bgn,  end);

  fprintf(stderr, "\n");
  fprintf(stderr, "LINEAR SEARCH the bucket %lu-%lu for suffix %s.\n", bgn, end, toHex(suffix));

  for (mid=bgn; mid < end; mid++) {
    tag = _sufData->get(mid);

    fprintf(stderr, "LINR bgn %8lu %8lu %8lu end -- dat %s\n",
            bgn, mid, end, toHex(tag));

    if (tag == suffix) {
      fprintf(stderr, "FOUND?\n");
      assert(0);
      return(true);
    }
  }

  fprintf(stderr, "\n");
  fprintf(stderr, "FAILED kmer   0x%s\n", toHex(kmer));
  fprintf(stderr, "FAILED prefix 0x%s\n", toHex(prefix));
  fprintf(stderr, "FAILED suffix 0x%s\n", toHex(suffix));
  fprintf(stderr, "\n");

  assert(0);
}

}  //  namespace merylutil::kmers::v1
