
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 2007, J. Craig Venter Institute.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received (LICENSE.txt) a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *************************************************************************/

static const char *rcsid = "$Id: AS_UTL_UID.c,v 1.13 2011-09-06 01:11:56 mkotelbajcvi Exp $";

#include "AS_UTL_UID.h"
#include "AS_PER_gkpStore.h"

static gkStore *AS_UID_gkp = NULL;

void
AS_UID_setGatekeeper(void *gkp) {
  AS_UID_gkp = (gkStore *)gkp;
}

static
gkStore *
AS_UID_getGatekeeper(void) {
  if (AS_UID_gkp == NULL)
    AS_UID_gkp = new gkStore;
  return(AS_UID_gkp);
}



//  A very common operation is to print a bunch of UIDs at the same
//  time.  We allow printing of up to 16 UIDs at the same time.
//
char *
AS_UID_toString(AS_UID uid) {
  static  int    localindex  = 0;
  static  char  *localbuffer = NULL;

  localindex++;

  if (localbuffer == NULL) {
    localindex  = 0;
    localbuffer = (char *)safe_malloc(sizeof(char) * 16 * (MAX_UID_LENGTH + 1));
  }

  if (localindex >= 16)
    localindex = 0;

  char *retbuffer = localbuffer + localindex * (MAX_UID_LENGTH + 1);

  if (uid.isString) {
    char  *uidstr = AS_UID_getGatekeeper()->gkStore_getUIDstring(uid);

    if (uidstr)
      sprintf(retbuffer, F_STR, uidstr);
    else
      sprintf(retbuffer, "CAx"F_U64, uid.UID);
  } else {
    sprintf(retbuffer, F_U64, uid.UID);
  }

  return(retbuffer);
}




//  Converts the next word in a string to a uid, returns a pointer to
//  the string after the uid.  If the uid is known to the assembler, a
//  valid uid is returned.  Otherwise, the uid holds a pointer to the
//  string containing the uid, and the uid should be passed to
//  AS_UID_load() to save it in the assembler.
//
AS_UID
AS_UID_lookup(char *uidstr, char **nxtstr) {
  AS_UID  uid = AS_UID_undefined();

  assert(uidstr != NULL);

  //  Bump up to the next word.
  //
  while (*uidstr && isspace(*uidstr))  uidstr++;

  //  If we are all numeric, and smaller than what will fit in
  //  63-bits, then we're numeric, otherwise, we're a string.
  //
  //  While we're here, check for commas.  UIDs cannot have commas in
  //  them -- as AFG, UTG, CCO, SCF and MDI all use constructs like
  //  "uid,iid".
  //
  //  BPW thought it would be easy to just replace commas with dots,
  //  but soon realized that (a) the load code needs the fix too, and
  //  (b) the replacement cannot be done in 'uidstr' (we'd be stepping
  //  on data that might be fragile).

  int   len = 0;
  int   dig = 0;
  int   com = 0;

  while (uidstr[len] && !isspace(uidstr[len])) {
    if (('0' <= uidstr[len]) && (uidstr[len] <= '9'))
      dig++;
    if (',' == uidstr[len])
      com++;
    len++;
  }

  if (len > MAX_UID_LENGTH) {
    fprintf(stderr, "ERROR:  UID '"F_STR"' larger than maximum allowed ("F_S32" letters).  FAIL.\n",
            uidstr, MAX_UID_LENGTH);
    exit(1);
  }

  if (com > 0) {
    fprintf(stderr, "ERROR:  UID '"F_STR"' contains "F_S32" commas.  FAIL.",
            uidstr, com);
    exit(1);
  }

  int  isInteger  = 0;
  int  isInternal = 0;

  //  If all digits, and waaay below 2^64 - 1, we're integer.
  if ((len == dig) &&
      (len < 19))
    isInteger = 1;

  //  If all digits, and barely below 2^64 - 1, we're integer.
  if ((len == dig) &&
      (len == 19) &&
      (strcmp(uidstr, "9223372036854775807") <= 0))
    isInteger = 1;

  //  Integer?  Convert.  Otherwise, look it up in gatekeeper.
  if (isInteger) {
    uid.isString  = 0;
    uid.UID       = strtoull(uidstr, NULL, 10);
  } else {
    uid = AS_UID_getGatekeeper()->gkStore_getUIDfromString(uidstr);
  }

  //  Now bump past the uid, almost like strtoull does.
  if (nxtstr) {
    while (*uidstr && !isspace(*uidstr))  uidstr++;
    while (*uidstr &&  isspace(*uidstr))  uidstr++;
    *nxtstr = uidstr;
  }

  return(uid);
}



//  Adds a uid to the gatekeeper store.  If the uid is already known
//  (or doesn't need to be saved -- if it's just an integer), the
//  function does nothing.  Do NOT use this for general UID lookups!
//  Use AS_UID_lookup() instead.
//
AS_UID
AS_UID_load(char *uidstr) {
  AS_UID  uid = AS_UID_lookup(uidstr, NULL);

  if ((uid.UID > 0) || (uid.isString == 1))
    return(uid);

  //  Brand new uid.  Add it.

  return(AS_UID_getGatekeeper()->gkStore_addUID(uidstr));
}
