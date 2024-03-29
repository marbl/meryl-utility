
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

#include "system.H"
#include "sweatShop-v1.H"

#include <pthread.h>
#include <sched.h>  //  pthread scheduling stuff


class sweatShopWorker {
public:
  sweatShopWorker() {
    shop            = 0L;
    threadUserData  = 0L;
    numComputed     = 0;
    workerQueue     = 0L;
    workerQueueLen  = 0L;
  };

  sweatShop        *shop;
  void             *threadUserData;
  pthread_t         threadID;
  uint32            numComputed;
  sweatShopState  **workerQueue;
  uint32            workerQueueLen;
};


//  This gets created by the loader, passed to the worker, and printed
//  by the writer.  userData is controlled by the user.
//
class sweatShopState {
public:
  sweatShopState(void *userData) {
    _user      = userData;
    _computed  = false;
    _outputted = false;
    _next      = 0L;
  };
  ~sweatShopState() {
  };

  void             *_user;
  bool              _computed;
  bool              _outputted;
  sweatShopState   *_next;
};




//  Simply forwards control to the class
void*
_sweatshop_loaderThread(void *ss_) {
  sweatShop *ss = (sweatShop *)ss_;
  return(ss->loader());
}

void*
_sweatshop_workerThread(void *sw_) {
  sweatShopWorker *sw = (sweatShopWorker *)sw_;
  return(sw->shop->worker(sw));
}

void*
_sweatshop_writerThread(void *ss_) {
  sweatShop *ss = (sweatShop *)ss_;
  return(ss->writer());
}

void*
_sweatshop_statusThread(void *ss_) {
  sweatShop *ss = (sweatShop *)ss_;
  return(ss->status());
}



sweatShop::sweatShop(void*(*loaderfcn)(void *G),
                     void (*workerfcn)(void *G, void *T, void *S),
                     void (*writerfcn)(void *G, void *S),
                     void (*statusfcn)(void *G, uint64 numberLoaded, uint64 numberComputed, uint64 numberOutput)) {

  _userLoader       = loaderfcn;
  _userWorker       = workerfcn;
  _userWriter       = writerfcn;
  _userStatus       = statusfcn;

  _globalUserData   = 0L;

  _writerP          = 0L;
  _workerP          = 0L;
  _loaderP          = 0L;

  _showStatus       = false;
  _writeInOrder     = true;

  _loaderQueueSize  = 1024;
  _loaderQueueMax   = 10240;
  _loaderQueueMin   = 4;  //  _numberOfWorkers * 2, reset when that changes
  _loaderBatchSize  = 1;
  _workerBatchSize  = 1;
  _writerQueueSize  = 4096;
  _writerQueueMax   = 10240;

  _numberOfWorkers  = 2;

  _workerData       = 0L;

  _numberLoaded     = 0;
  _numberComputed   = 0;
  _numberOutput     = 0;
}


sweatShop::~sweatShop() {
  delete [] _workerData;
}



void
sweatShop::setThreadData(uint32 t, void *x) {
  if (_workerData == 0L)
    _workerData = new sweatShopWorker [_numberOfWorkers];

  if (t >= _numberOfWorkers)
    fprintf(stderr, "sweatShop::setThreadData()-- worker ID " F_U32 " more than number of workers=" F_U32 "\n", t, _numberOfWorkers), exit(1);

  _workerData[t].threadUserData = x;
}



//  Build a list of states to add in one swoop
//
void
sweatShop::loaderAddToLocal(sweatShopState *&tail, sweatShopState *&head, sweatShopState *thisState) {

  thisState->_next  = 0L;

  if (tail) {
    head->_next = thisState;
    head        = thisState;
  } else {
    tail = head = thisState;
  }
}


//  Add a bunch of new states to the queue.
//
void
sweatShop::loaderAppendToGlobal(sweatShopState *&tail, sweatShopState *&head, uint32 num) {
  int err;

  if ((tail == 0L) || (head == 0L))
    return;

  err = pthread_mutex_lock(&_stateMutex);
  if (err != 0)
    fprintf(stderr, "sweatShop::loaderAppend()--  Failed to lock mutex (%d).  Fail.\n", err), exit(1);

  if (_loaderP == 0L) {
    _writerP      = tail;
    _workerP      = tail;
    _loaderP      = head;
  } else {
    _loaderP->_next = tail;
  }
  _loaderP        = head;

  _numberLoaded += num;

  err = pthread_mutex_unlock(&_stateMutex);
  if (err != 0)
    fprintf(stderr, "sweatShop::loaderAppend()--  Failed to unlock mutex (%d).  Fail.\n", err), exit(1);

  tail = 0L;
  head = 0L;
}



void*
sweatShop::loader(void) {
  struct timespec   naptime;
  sweatShopState   *tail       = nullptr;  //  A local list, to reduce the number of times we
  sweatShopState   *head       = nullptr;  //  lock the global list.
  uint32            numLoaded  = 0;

  naptime.tv_sec      = 0;
  naptime.tv_nsec     = 166666666ULL;  //  1/6 second

  while (1) {
    void *object = NULL;

    while (_numberLoaded > _numberComputed + _loaderQueueSize)  //  Sleep if the queue is too big.
      nanosleep(&naptime, 0L);

    //  If a userLoader function exists, use it to load the data object, then
    //  make a new state for that object.

    if (_userLoader)
      object = (*_userLoader)(_globalUserData);

    sweatShopState  *thisState = new sweatShopState(object);

    //  If there is no user pointer, we've run out of inputs.
    //  Push on the empty state to the local list, force an append
    //  to the global list, and exit this loader function.

    if (thisState->_user == nullptr) {
      loaderAddToLocal(tail, head, thisState);
      loaderAppendToGlobal(tail, head, numLoaded + 1);

      return(nullptr);
    }

    //  Otherwise, we've loaded a user object.  Push it onto the local list,
    //  then merge into the global list if the local list is long enough.

    loaderAddToLocal(tail, head, thisState);
    numLoaded++;

    if (numLoaded >= _loaderBatchSize) {
      loaderAppendToGlobal(tail, head, numLoaded);
      numLoaded = 0;
    }
  }

  return(nullptr);  //  Never returns.
}



void*
sweatShop::worker(sweatShopWorker *workerData) {

  struct timespec   naptime;
  naptime.tv_sec      = 0;
  naptime.tv_nsec     = 50000000ULL;

  bool    moreToCompute = true;
  int     err;

  while (moreToCompute) {

    //  Usually beacuse some worker is taking a long time, and the
    //  output queue isn't big enough.
    //
    while (_numberOutput + _writerQueueSize < _numberComputed)
      nanosleep(&naptime, 0L);

    //  Grab the next state.  We don't grab it if it's the last in the
    //  queue (else we would fall off the end) UNLESS it really is the
    //  last one.
    //
    err = pthread_mutex_lock(&_stateMutex);
    if (err != 0)
      fprintf(stderr, "sweatShop::worker()--  Failed to lock mutex (%d).  Fail.\n", err), exit(1);

    for (workerData->workerQueueLen = 0; ((workerData->workerQueueLen < _workerBatchSize) &&
                                          (_workerP) &&
                                          ((_workerP->_next != 0L) || (_workerP->_user == 0L))); workerData->workerQueueLen++) {
      workerData->workerQueue[workerData->workerQueueLen] = _workerP;
      _workerP = _workerP->_next;
    }

    if (_workerP == 0L)
      moreToCompute = false;

    err = pthread_mutex_unlock(&_stateMutex);
    if (err != 0)
      fprintf(stderr, "sweatShop::worker()--  Failed to lock mutex (%d).  Fail.\n", err), exit(1);


    if (workerData->workerQueueLen == 0) {
      //  No work, sleep a bit to prevent thrashing the mutex and resume.
      nanosleep(&naptime, 0L);
      continue;
    }

    //  Execute
    //
    for (uint32 x=0; x<workerData->workerQueueLen; x++) {
      sweatShopState *ts = workerData->workerQueue[x];

      if (ts && ts->_user) {
        if (_userWorker)
          (*_userWorker)(_globalUserData, workerData->threadUserData, ts->_user);
        ts->_computed = true;
        workerData->numComputed++;
      } else {
        //  When we really do run out of stuff to do, we'll end up here
        //  (only one thread will end up in the other case, with
        //  something to do and moreToCompute=false).  If it's actually
        //  the end, skip the sleep and just get outta here.
        //
        if (moreToCompute == true) {
          fprintf(stderr, "WARNING!  Worker is sleeping because the reader is slow!\n");
          nanosleep(&naptime, 0L);
        }
      }
    }
  }

  //fprintf(stderr, "sweatShop::worker exits.\n");
  return(0L);
}


void
sweatShop::writerWrite(sweatShopState *w) {

  if (_userWriter)
    (*_userWriter)(_globalUserData, w->_user);
  _numberOutput++;

  w->_outputted = true;
}


void*
sweatShop::writer(void) {
  sweatShopState  *deleteState = 0L;
  struct timespec naptime1 = { .tv_sec = 0, .tv_nsec = 5000000ULL };
  struct timespec naptime2 = { .tv_sec = 0, .tv_nsec = 5000000ULL };


  while ((_writerP        != nullptr) &&
         (_writerP->_user != nullptr)) {

    //  If a complete result, write it.
    if ((_writerP->_computed  == true) &&
        (_writerP->_outputted == false)) {
      writerWrite(_writerP);
      continue;
    }

    //  If we can write output out-of-order, search ahead
    //  for any results and output them.
    //  if (_outOfOrder == true)
    if (_writeInOrder == false) {
      for (sweatShopState *ss = _writerP; ss != nullptr; ss = ss->_next)
        if ((ss->_computed  == true) &&
            (ss->_outputted == false)) {
          writerWrite(ss);
        }
    }

    //  If no next, wait for input to appear.  We can't purge this node
    //  from the list until there is a next, else we lose the list!
    if (_writerP->_next == nullptr) {
      nanosleep(&naptime1, 0L);
      continue;
    }

    //  If already output, remove the node.
    if (_writerP->_outputted == true) {
      sweatShopState *ds = _writerP;
      _writerP           = _writerP->_next;

      delete ds;
      continue;
    }

    //  Otherwise, we need to wait for a state to appear on the queue.
    nanosleep(&naptime2, 0L);
  }

  //  Tell status to stop.
  _writerP = 0L;

  return(0L);
}


//  This thread not only shows a status message, but it also updates the critical shared variable
//  _numberComputed.  Worker threads use this to throttle themselves.  Thus, even if _showStatus is
//  not set, and this thread doesn't _appear_ to be doing anything useful....it is.
//

void
sweatShopStatus(double startTime, uint64 numberLoaded, uint64 numberComputed, uint64 numberOutput) {
  double thisTime  = getTime();
  uint64 deltaOut  = 0;
  uint64 deltaCPU  = 0;
  double cpuPerSec = 0;

  if (numberComputed > numberOutput)
    deltaOut = numberComputed - numberOutput;
  if (numberLoaded > numberComputed)
    deltaCPU = numberLoaded - numberComputed;

  cpuPerSec = numberComputed / (thisTime - startTime);

  fprintf(stderr, " %6.1f/s - %8" F_U64P " loaded; %8" F_U64P " queued for compute; %8" F_U64P " finished; %8" F_U64P " written; %8" F_U64P " queued for output)\r",
          cpuPerSec, numberLoaded, deltaCPU, numberComputed, numberOutput, deltaOut);
}



void*
sweatShop::status(void) {
  struct timespec   naptime;
  naptime.tv_sec      = 0;
  naptime.tv_nsec     = 250000000ULL;

  double  startTime  = getTime() - 0.001;
  uint64  readjustAt = 16384;

  while (_writerP) {
    uint32 nc = 0;
    for (uint32 i=0; i<_numberOfWorkers; i++)
      nc += _workerData[i].numComputed;
    _numberComputed = nc;

    if (_showStatus) {
      if (_userStatus)
        (*_userStatus)(_globalUserData, _numberLoaded, _numberComputed, _numberOutput);
      else
        sweatShopStatus(startTime, _numberLoaded, _numberComputed, _numberOutput);

      fflush(stderr);
    }

    //  Readjust queue sizes based on current performance, but don't let it get too big or small.
    //  In particular, don't let it get below 2*numberOfWorkers.
    //
     if (_numberComputed > readjustAt) {
       double cpuPerSec = _numberComputed / (getTime() - startTime);

       readjustAt       += (uint64)(2 * cpuPerSec);
       _loaderQueueSize  = (uint32)(5 * cpuPerSec);
     }

    if (_loaderQueueSize < _loaderQueueMin)
      _loaderQueueSize = _loaderQueueMin;

    if (_loaderQueueSize < 2 * _numberOfWorkers)
      _loaderQueueSize = 2 * _numberOfWorkers;

    if (_loaderQueueSize > _loaderQueueMax)
      _loaderQueueSize = _loaderQueueMax;

    nanosleep(&naptime, 0L);
  }

  //  Call the status function, giving it:
  //    numberLoaded
  //    numberComputed
  //    numberOutput

  if (_showStatus) {
    if (_userStatus)
      (*_userStatus)(_globalUserData, _numberLoaded, _numberComputed, _numberOutput);
    else
      sweatShopStatus(startTime, _numberLoaded, _numberComputed, _numberOutput);

    fprintf(stderr, "\n");
    fflush(stderr);
  }

  //fprintf(stderr, "sweatShop::status exits.\n");
  return(0L);
}





void
sweatShop::run(void *user, bool beVerbose) {
  pthread_attr_t      threadAttr;
  pthread_t           threadIDloader;
  pthread_t           threadIDwriter;
  pthread_t           threadIDstats;
#if 0
  int                 threadSchedPolicy = 0;
  struct sched_param  threadSchedParamDef;
  struct sched_param  threadSchedParamMax;
#endif
  int                 err = 0;

  _globalUserData = user;
  _showStatus     = beVerbose;

  //  Configure everything ahead of time.

  if (_workerBatchSize < 1)
    _workerBatchSize = 1;

  if (_workerData == 0L)
    _workerData = new sweatShopWorker [_numberOfWorkers];

  for (uint32 i=0; i<_numberOfWorkers; i++) {
    _workerData[i].shop        = this;
    _workerData[i].workerQueue = new sweatShopState * [_workerBatchSize];
  }

  //  Open the doors.

  errno = 0;

  err = pthread_mutex_init(&_stateMutex, NULL);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (state mutex): %s.\n", strerror(err)), exit(1);

  err = pthread_attr_init(&threadAttr);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (attr init): %s.\n", strerror(err)), exit(1);

  err = pthread_attr_setscope(&threadAttr, PTHREAD_SCOPE_SYSTEM);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (set scope): %s.\n", strerror(err)), exit(1);

  err = pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_JOINABLE);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (joinable): %s.\n", strerror(err)), exit(1);

#if 0
  err = pthread_attr_getschedparam(&threadAttr, &threadSchedParamDef);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (get default param): %s.\n", strerror(err)), exit(1);

  err = pthread_attr_getschedparam(&threadAttr, &threadSchedParamMax);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (get max param): %s.\n", strerror(err)), exit(1);
#endif

  //  SCHED_RR needs root privs to run on FreeBSD.
  //
  //err = pthread_attr_setschedpolicy(&threadAttr, SCHED_RR);
  //if (err)
  //  fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (sched policy): %s.\n", strerror(err)), exit(1);

#if 0
  err = pthread_attr_getschedpolicy(&threadAttr, &threadSchedPolicy);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to configure pthreads (sched policy): %s.\n", strerror(err)), exit(1);

  errno = 0;
  threadSchedParamMax.sched_priority = sched_get_priority_max(threadSchedPolicy);
  if (errno)
    fprintf(stderr, "sweatShop::run()--  WARNING: Failed to configure pthreads (set max param priority): %s.\n", strerror(errno));

  //  Fire off the loader

  err = pthread_attr_setschedparam(&threadAttr, &threadSchedParamMax);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to set loader priority: %s.\n", strerror(err)), exit(1);
#endif

  err = pthread_create(&threadIDloader, &threadAttr, _sweatshop_loaderThread, this);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to launch loader thread: %s.\n", strerror(err)), exit(1);

  //  Wait for it to actually load something (otherwise all the
  //  workers immediately go home)

  while (!_writerP && !_workerP && !_loaderP) {
    struct timespec   naptime;
    naptime.tv_sec      = 0;
    naptime.tv_nsec     = 250000ULL;
    nanosleep(&naptime, 0L);
  }

  //  Start the statistics and writer

#if 0
  err = pthread_attr_setschedparam(&threadAttr, &threadSchedParamMax);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to set status and writer priority: %s.\n", strerror(err)), exit(1);
#endif

  err = pthread_create(&threadIDstats,  &threadAttr, _sweatshop_statusThread, this);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to launch status thread: %s.\n", strerror(err)), exit(1);

  err = pthread_create(&threadIDwriter, &threadAttr, _sweatshop_writerThread, this);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to launch writer thread: %s.\n", strerror(err)), exit(1);

  //  And some labor

#if 0
  err = pthread_attr_setschedparam(&threadAttr, &threadSchedParamDef);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to set worker priority: %s.\n", strerror(err)), exit(1);
#endif

  for (uint32 i=0; i<_numberOfWorkers; i++) {
    err = pthread_create(&_workerData[i].threadID, &threadAttr, _sweatshop_workerThread, _workerData + i);
    if (err)
      fprintf(stderr, "sweatShop::run()--  Failed to launch worker thread " F_U32 ": %s.\n", i, strerror(err)), exit(1);
  }

  //  Now sit back and relax.

  err = pthread_join(threadIDloader, 0L);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to join loader thread: %s.\n", strerror(err)), exit(1);

  err = pthread_join(threadIDwriter, 0L);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to join writer thread: %s.\n", strerror(err)), exit(1);

  err = pthread_join(threadIDstats,  0L);
  if (err)
    fprintf(stderr, "sweatShop::run()--  Failed to join status thread: %s.\n", strerror(err)), exit(1);

  for (uint32 i=0; i<_numberOfWorkers; i++) {
    err = pthread_join(_workerData[i].threadID, 0L);
    if (err)
      fprintf(stderr, "sweatShop::run()--  Failed to join worker thread " F_U32 ": %s.\n", i, strerror(err)), exit(1);
  }

  //  Cleanup.

  pthread_attr_destroy(&threadAttr);

  for (uint32 i=0; i<_numberOfWorkers; i++)
    delete [] _workerData[i].workerQueue;

  delete _loaderP;
  _loaderP = _workerP = _writerP = 0L;
}
