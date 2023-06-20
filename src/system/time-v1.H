
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

#ifndef TIME_H
#define TIME_H

#include "types.H"

namespace merylutil::inline system::inline v1 {

//
//  The meryl-utility time object.
//
//  It serves to store both
//   - real time
//   - delta time (real time minus some base time)
//   - file time (both update and access times)
//

class muTime {
public:
  muTime() {
    getTime(0);
    getTime(1);
  }
  muTime(char const *prefix, char separator='.', char const *suffix=nullptr) {
    getTimeOfFile(prefix, separator, suffix);
  }
  muTime(FILE *F) {
    getTimeOfFile(F);
  }

public:
  int64     years(void)   { if (_yrs == int64max) updateYDHMS(); return _yrs; }    //  These return the integer number of (unit)s
  int64     days(void)    { if (_yrs == int64max) updateYDHMS(); return _dys; };   //  after subtracting out the larger units.
  int64     hours(void)   { if (_yrs == int64max) updateYDHMS(); return _hrs; };   //  Example: for a time of 181 seconds:
  int64     minutes(void) { if (_yrs == int64max) updateYDHMS(); return _mns; };   //    minutes() = 3
  double    seconds(void) { if (_yrs == int64max) updateYDHMS(); return _scs; };   //    seconds() = 1.0

public:
  double    full_years(void)   { return ttt() / _years;   }   //  These return the absolue quantity of
  double    full_days(void)    { return ttt() / _days;    }   //  each unit.  Example: for 181 seconds:
  double    full_hours(void)   { return ttt() / _hours;   }   //    full_hours()   = 0.0502777
  double    full_minutes(void) { return ttt() / _minutes; }   //    full_minutes() = 3.01666
  double    full_seconds(void) { return ttt() / _seconds; }   //    full_seconds() = 181.0

  //  setTimeOfFile()
  //   - Will set the modification time to the time in the object.  If
  //     getTimeOfFile() was used to set the object time, then the access
  //     time is also set.
  //
  //  getTimeOfFile()
  //   - Will set the object time to the modification time of the file.
  //     The access time is also saved.
  //
  bool      setTimeOfFile(char const *prefix, char separator='.', char const *suffix=nullptr);
  muTime   &getTimeOfFile(char const *prefix, char separator='.', char const *suffix=nullptr);
  muTime   &getTimeOfFile(FILE *F);

  //  getTime()
  //   - Will set the specified time index to the current time.
  //     By convention:
  //       [0] - base time
  //       [1] - primary time and file update time
  //       [2] -                  file access time
  //
  muTime   &getTime(uint32 idx=1) {
    struct timeval  tp;

    gettimeofday(&tp, nullptr);

    _s[idx] = tp.tv_sec;
    _n[idx] = tp.tv_usec * 1000;

    _yrs = int64max;
    _dys = int64max;
    _hrs = int64max;
    _mns = int64max;
    _scs = DBL_MAX;

    return *this;
  }

  //  setStartTime()
  //   - For interval timing, this will set the base time to the present time.
  //
  muTime   &setStartTime(void) { return getTime(0); }

  //  Comparison operators all work on 'delta time'.
  //
  bool    operator< (muTime &that) { return (_s[1]-_s[0] <  that._s[1]-that._s[0]) || (_s[1]-_s[0] == that._s[1]-that._s[0] && _n[1]-_n[0] <  that._n[1]-that._n[0]); }
  bool    operator> (muTime &that) { return (_s[1]-_s[0] >  that._s[1]-that._s[0]) || (_s[1]-_s[0] == that._s[1]-that._s[0] && _n[1]-_n[0] >  that._n[1]-that._n[0]); }
  bool    operator<=(muTime &that) { return (_s[1]-_s[0] <= that._s[1]-that._s[0]) || (_s[1]-_s[0] == that._s[1]-that._s[0] && _n[1]-_n[0] <= that._n[1]-that._n[0]); }
  bool    operator>=(muTime &that) { return (_s[1]-_s[0] >= that._s[1]-that._s[0]) || (_s[1]-_s[0] == that._s[1]-that._s[0] && _n[1]-_n[0] >= that._n[1]-that._n[0]); }

  bool    operator==(muTime &that) { return (_s[1]-_s[0] == that._s[1]-that._s[0] && _n[1]-_n[0] == that._n[1]-that._n[0]); }
  bool    operator!=(muTime &that) { return (_s[1]-_s[0] != that._s[1]-that._s[0] || _n[1]-_n[0] != that._n[1]-that._n[0]); }

private:
  static constexpr double _seconds =   1.0;
  static constexpr double _minutes =  60.0 * _seconds;
  static constexpr double _hours   =  60.0 * _minutes;
  static constexpr double _days    =  24.0 * _hours;
  static constexpr double _years   = 365.0 * _days;

  double    ttt(void) {
    return((double)(_s[1] - _s[0]) +
           (double)(_n[1] - _n[0]) / 1000000000.0);
  }

  void      updateYDHMS(void) {
    double t = ttt();

    _yrs = (int64)floor(t / _years);    t -= _yrs * _years;
    _dys = (int64)floor(t / _days);     t -= _dys * _days;
    _hrs = (int64)floor(t / _hours);    t -= _yrs * _hours;
    _mns = (int64)floor(t / _minutes);  t -= _yrs * _minutes;
    _scs =              t;
  }

  int64   _s[3] = { 0, 0, UTIME_OMIT };
  int64   _n[3] = { 0, 0, UTIME_OMIT };

  int64   _yrs = int64max;
  int64   _dys = int64max;
  int64   _hrs = int64max;
  int64   _mns = int64max;
  double  _scs = DBL_MAX;
};

}  //  namespace merylutil::system::v1

#endif
