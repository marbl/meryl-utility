/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 2005, J. Craig Venter Institute. All rights reserved.
 * Author: Brian Walenz
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

#ifndef RUNTIMEEXCEPTION_H
#define RUNTIMEEXCEPTION_H

static const char* rcsid_RUNTIMEEXCEPTION_H = "$Id: RuntimeException.h,v 1.8 2011-09-05 16:49:45 mkotelbajcvi Exp $";

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

using namespace std;

#include "AS_global.h"
#include "ExceptionUtils.h"
#include "StackTrace.h"
#include "StringUtils.h"

using namespace Utility;

class RuntimeException : public exception
{
public:
	static const uint32 DEFAULT_CAUSE_DEPTH = 3;
	
	virtual const char* what() const throw();
	virtual string toString(string& buffer, uint32 depth = 0) const throw();

	operator const char*()
	{
		return this->what();
	}
	
	string getMessage()
	{
		return this->message;
	}
	
	RuntimeException* getCause()
	{
		return this->cause;
	}
	
	StackTrace* getStackTrace()
	{
		return this->stackTrace;
	}

protected:
	string message;
	RuntimeException* cause;
	StackTrace* stackTrace;
	
	RuntimeException(string message = "", RuntimeException* cause = NULL) throw();
	~RuntimeException() throw();
};

#endif
