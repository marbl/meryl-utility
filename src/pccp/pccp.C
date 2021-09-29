#include "runtime.H"
#include "system.H"
#include "types.H"
#include "files.H"
#include "md5.H"

#include "sweatShop.H"

#include <fcntl.h>
#include <sys/stat.h>
#include <vector>


class cpBuf {
public:
  cpBuf(FILE *F, uint32 max) {
    _b    = new uint8 [max];
    _bLen = loadFromFile(_b, "stuff", max, F, false);
  }

  ~cpBuf() {
    delete [] _b;
  }

  uint32  _bLen = 0;
  uint8  *_b    = nullptr;
};



class cpBufState {
public:
  cpBufState(char const *inname, char const *otpath) {
    snprintf(inPath, FILENAME_MAX, "%s", inname);
    snprintf(otPath, FILENAME_MAX, "%s/%s", otpath, basename(inname));

    fileSize = AS_UTL_sizeOfFile(inname);

    inFile = AS_UTL_openInputFile(inPath);
    otFile = AS_UTL_openOutputFile(otPath);
  };

  ~cpBufState() {
    struct stat     st;
    struct timespec times[2];   // access, modification

    AS_UTL_closeFile(inFile);
    AS_UTL_closeFile(otFile);

    errno = 0;
    if (stat(inPath, &st)) {
      fprintf(stderr, "stat error: %s\n", strerror(errno));
    }

    times[0].tv_sec  = st.st_atim.tv_sec;
    times[0].tv_nsec = st.st_atim.tv_nsec;

    times[1].tv_sec  = st.st_mtim.tv_sec;
    times[1].tv_nsec = st.st_mtim.tv_nsec;

    errno = 0;
    if (utimensat(AT_FDCWD, otPath, times, 0)) {
      fprintf(stderr, "futimens error: %s\n", strerror(errno));
    }
  };

  char const *basename(char const *name) {
    uint32 l = strlen(name);

    for (uint32 ii=l; --ii; ) {
      if (name[ii] == '/')
        return(name + ii + 1);
    }

    return(name);
  }

  cpBuf      *read(void) {
    cpBuf  *b = new cpBuf(inFile, 1048576);
    double  n = getTime();

    inStart = std::min(inStart, n);
    inEnd   = std::max(inEnd,   n);

    if (b->_bLen == 0) {
      inEnd = getTime();
      delete b;
      return(nullptr);
    }

    return(b);
  };

  void        write(cpBuf *b) {
    double  n = getTime();

    otStart = std::min(otStart, n);
    otEnd   = std::max(otEnd,   n);

    writeToFile(b->_b, "stuff", b->_bLen, otFile);

    delete b;
  }

  char        inPath[FILENAME_MAX];
  char        otPath[FILENAME_MAX];

  FILE       *inFile = nullptr;
  FILE       *otFile = nullptr;

  uint64     bufMax   = 1048576;
  uint64     fileSize = 0;

  double     inStart=DBL_MAX, inEnd=0;
  double     otStart=DBL_MAX, otEnd=0;

  md5sum     md5;
};




void *bufReader(void *G) {
  cpBufState *g = (cpBufState *)G;
  return(g->read());
}

void  bufWorker(void *G, void *T, void *S) {
  cpBufState *g = (cpBufState *)G;
  cpBuf      *s = (cpBuf      *)S;

  g->md5.addBlock(s->_b, s->_bLen);
}

void  bufWriter(void *G, void *S) {
  cpBufState *g = (cpBufState *)G;
  cpBuf      *s = (cpBuf      *)S;
  g->write(s);
}

void  bufStatus(void *G, uint64 numberLoaded, uint64 numberComputed, uint64 numberOutput) {
  cpBufState *g = (cpBufState *)G;

  double  thisTime = getTime();

  double  inSize  = numberLoaded * g->bufMax;
  double  inPerc  = 100.0 * inSize / g->fileSize;
  double  inSpeed =         inSize / (g->inEnd - g->inStart);

  double  otSize  = numberOutput * g->bufMax;
  double  otPerc  = 100.0 * otSize / g->fileSize;
  double  otSpeed =         otSize / (g->otEnd - g->otStart);

  if (inPerc > 100)   inPerc = 100.0;
  if (otPerc > 100)   otPerc = 100.0;

#if 0
  fprintf(stderr, "  %6.2f%%  %6.2f MB  %6.2f MB/sec -> %6.2f MB -> %6.2f%%  %6.2f MB  %6.2f MB/sec\r",
          inPerc, inSize / 1048576.0, inSpeed / 1048576.0,
          (inSize - otSize) / 1048576.0,
          otPerc, otSize / 1048576.0, otSpeed / 1048576.0);
#else
  fprintf(stderr, "\033[3A");
  fprintf(stderr, "  INPUT:  %6.2f%%  %8.2f MB  %6.2f MB/sec\n", inPerc, inSize / 1048576.0, inSpeed / 1048576.0);
  fprintf(stderr, "  BUFFER:          %8.2f MB\n", (inSize - otSize) / 1048576.0);
  fprintf(stderr, "  OUTPUT: %6.2f%%  %8.2f MB  %6.2f MB/sec\n",     otPerc, otSize / 1048576.0, otSpeed / 1048576.0);
#endif
}


int
main(int argc, char **argv) {
  std::vector<char const *>   errors;
  std::vector<char const *>   infiles;
  char const                 *otpath  = nullptr;

  for (int arg=1; arg<argc; arg++) {
    if      (fileExists(argv[arg]))
      infiles.push_back(argv[arg]);

    else if (directoryExists(argv[arg]) && (arg == argc-1))
      otpath = argv[arg];

    else
      sprintf(errors, "ERROR: '%s' is neither an input file nor an output directory.\n", argv[arg]);
  }

  if (infiles.size() == 0)
    sprintf(errors, "ERROR: no input-files supplied.\n");
  if (otpath == nullptr)
    sprintf(errors, "ERROR: no output-directory supplied.\n");

  if (errors.size() > 0) {
    fprintf(stderr, "usage: %s <input-files ...> <output-directory>\n", argv[0]);
    fprintf(stderr, "\n");
    for (char const *e : errors)
      fputs(e, stderr);

    return(1);
  }

  for (char const *infile : infiles) {
    cpBufState *g  = new cpBufState(infile, otpath);
    sweatShop  *ss = new sweatShop(bufReader, bufWorker, bufWriter, bufStatus);

    ss->setLoaderQueueSize(128);    //  Just comuting the md5, so we don't need lots of input buffering.
    ss->setNumberOfWorkers(1);      //
    ss->setWriterQueueSize(16384);  //  But if we wait for writing, keep filling the queue.
    ss->setInOrderOutput(true);

    fprintf(stderr, "%s -> %s\n", infile, otpath);
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "\n");

    ss->run(g, true);

    g->md5.finalize();

    fprintf(stderr, "\033[3A");
    fprintf(stderr, "  MD5:    %s\n", g->md5.toString());
    fprintf(stderr, "\033[3B");

    delete ss;
    delete g;
  }

  return(0);
}