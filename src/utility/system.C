
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

#include "types.H"
#include "system.H"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#if defined(__FreeBSD__)
#include <stdlib.h>
#include <malloc_np.h>
#endif

#if defined(JEMALLOC)
#include "jemalloc/jemalloc.h"
#endif



double
getTime(void) {
  struct timeval  tp;
  gettimeofday(&tp, NULL);
  return(tp.tv_sec + (double)tp.tv_usec / 1000000.0);
}



static
bool
getrusage(struct rusage &ru) {

  errno = 0;

  if (getrusage(RUSAGE_SELF, &ru) == -1) {
    fprintf(stderr, "getrusage(RUSAGE_SELF, ...) failed: %s\n",
            strerror(errno));
    return(false);
  }

  return(true);
}



static
bool
getrlimit(struct rlimit &rl) {

  errno = 0;

  if (getrlimit(RLIMIT_DATA, &rl) == -1) {
    fprintf(stderr, "getrlimit(RLIMIT_DATA, ...) failed: %s\n",
            strerror(errno));
    return(false);
  }

  return(true);
}



double
getCPUTime(void) {
  struct rusage  ru;
  double         tm = 0;

  if (getrusage(ru) == true)
    tm  = ((ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1000000.0) +
           (ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1000000.0));

  return(tm);
}



double
getProcessTime(void) {
  struct timeval tp;
  static double  st = 0.0;
  double         tm = 0;

  if (gettimeofday(&tp, NULL) == 0)
    tm  = tp.tv_sec + tp.tv_usec / 1000000.0;

  if (st == 0.0)
    st = tm;

  return(tm - st);
}



uint64
getProcessSize(void) {
  struct rusage  ru;
  uint64         sz = 0;

  if (getrusage(ru) == true) {
    sz  = ru.ru_maxrss;
#ifndef __APPLE__     //  Everybody but MacOS returns kilobytes.
    sz *= 1024;       //  MacOS returns bytes.
#endif
  }

  return(sz);
}



uint64
getProcessSizeLimit(void) {
  struct rlimit rl;
  uint64        sz = ~uint64ZERO;

  if (getrlimit(rl) == true)
    sz = rl.rlim_cur;

  return(sz);
}



uint64
getBytesAllocated(void) {
  uint64 epoch     = 1;
  size_t epochLen  = sizeof(uint64);
  size_t active    = 0;
  size_t activeLen = sizeof(size_t);

#if defined(__FreeBSD__) || defined(JEMALLOC)

  mallctl("epoch", NULL, NULL, &epoch, epochLen);
  mallctl("stats.active", &active, &activeLen, NULL, 0);

#else

  active = getProcessSize();

#endif

  return(active);
}



uint64
getPhysicalMemorySize(void) {
  uint64  physPages  = sysconf(_SC_PHYS_PAGES);
  uint64  pageSize   = sysconf(_SC_PAGESIZE);
  uint64  physMemory = physPages * pageSize;

  return(physMemory);
}



//  Return the size of a page of memory.  Every OS we care about (MacOS,
//  FreeBSD, Linux) claims to have getpagesize().
//
uint64
getPageSize(void) {
  return(getpagesize());
}




//  The next three functions query the environment and OpenMP to determine
//  how much memory is allowed to be used, and how many threads can be
//  created.
//

uint64
getMaxMemoryAllowed(void) {
  char    *env;
  uint64   maxmem = getPhysicalMemorySize();

  //
  //  SLURM_MEM_PER_CPU
  //    Same as --mem-per-cpu 
  //    "SLURM_MEM_PER_CPU=2048" for a request of --mem-per-cpu=2g
  //  SLURM_MEM_PER_GPU
  //    Requested memory per allocated GPU. Only set if the --mem-per-gpu option is specified. 
  //  SLURM_MEM_PER_NODE
  //    Same as --mem
  //    "SLURM_MEM_PER_NODE=5120" for a request of --mem=5g

  env = getenv("SLURM_MEM_PER_CPU");
  if (env)
    maxmem = getMaxThreadsAllowed() * strtouint64(env) * 1024 * 1024;

  env = getenv("SLURM_MEM_PER_NODE");
  if (env)
    maxmem = strtouint64(env) * 1024 * 1024;

  //
  //  There doesn't appear to be a comparable environment variable for SGE.
  //  I didn't look for PBS/OpenPBS/PBS Pro.
  //

  return(maxmem);
}



uint32
getMaxThreadsAllowed(void) {
  char    *env;
  uint32   nAllowed = 0;

  //  Check for Slurm variables.  (from sbatch man page)
  //    SLURM_CPUS_ON_NODE
  //     - Number of CPUS on the allocated node.
  //
  //    SLURM_JOB_CPUS_PER_NODE
  //     - --cpus-per-node
  //     - Count of processors available to the job on this node. Note the
  //       select/linear plugin allocates entire nodes to jobs, so the value
  //       indicates the total count of CPUs on the node. The select/cons_res
  //       plugin allocates individual processors to jobs, so this number
  //       indicates the number of processors on this node allocated to the
  //       job.
  //
  //    SLURM_JOB_NUM_NODES
  //     - total number of nodes in the job's resource allocation
  //
  //  SLRUM_MEM_PER_NODE is set if --mem is set
  //  SLURM_MEM_PER_CPU  is set if --mem-per-cpu is set
  //
  //  CPUS_ON_NODE == JOB_CPUS_PER_NODE
  //
  env = getenv("SLURM_JOB_CPUS_PER_NODE");
  if (env)
    nAllowed = strtouint32(env);

  //  Check for PBS/OpenPBS/PBS Pro variables.  (from Torque 9.0.3)
  //    PBS_NUM_NODES - Number of nodes allocated to the job
  //    PBS_NUM_PPN   - Number of procs per node allocated to the job
  //    PBS_NP        - Number of execution slots (cores) for the job (=== SLURM_TASKS_PER_NODE)
  //
  env = getenv("PBS_NUM_PPN");
  if (env)
    nAllowed = strtouint32(env);

  //  Check for SGE variables.
  //    NSLOTS
  //

  env = getenv("NSLOTS");
  if (env)
    nAllowed = strtouint32(env);

  //  Check for OpenMP variables.
  //    OMP_NUM_THREADS
  //

  env = getenv("OMP_NUM_THREADS");
  if (env)
    nAllowed = strtouint32(env);

  if (nAllowed == 0)
    nAllowed = omp_get_max_threads();

  return(nAllowed);
}



uint32
getNumThreadsActive(void) {
  return(omp_get_num_threads());
}

