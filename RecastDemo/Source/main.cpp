//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "RecastTool.h"

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#ifdef WIN32
#	define snprintf _snprintf
#	define putenv _putenv
#endif

int width = 1440, height = 900;

int main(int argc, char* argv[])
{
	bool presentationMode = false;
	RecastTool* tool = new RecastTool(width, height, presentationMode);
	tool->Run();
	delete tool;

	return 0;
}
