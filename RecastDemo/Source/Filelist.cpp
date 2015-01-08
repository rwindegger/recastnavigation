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

#include "Filelist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#ifdef WIN32
#	include <io.h>
#else
#	include <dirent.h>
#endif

void FileList::Add(const char* path)
{
	int n = strlen(path);
	char* copiedPath = new char[n + 1];
	strcpy(copiedPath, path);
	files.push_back(copiedPath);
}

void FileList::Clear()
{
	vector<char*>::iterator filesIter = files.begin();
	vector<char*>::iterator filesEnd = files.end();
	for (; filesIter != filesEnd; ++filesIter) {
		delete[] (*filesIter);
	}

	files.clear();
}

FileList::FileList()
{
}

FileList::~FileList()
{
	Clear();
}
	
void FileList::scanDirectory(const char* path, const char* ext)
{
	Clear();
	
#ifdef WIN32
	_finddata_t dir;
	char pathWithExt[260];
	sprintf_s(pathWithExt, "%s/*%s", path, ext);

	long fh = _findfirst(pathWithExt, &dir);
	if (fh == -1L)
	{
		return;
	}
	
	do
	{
		Add(dir.name);
	}
	while (_findnext(fh, &dir) == 0);
	_findclose(fh);
#else
	dirent* current = 0;
	DIR* dp = opendir(path);
	if (!dp)
	{
		return;
	}
	
	while ((current = readdir(dp)) != 0)
	{
		int len = strlen(current->d_name);
		if (len > 4 && strncmp(current->d_name+len-4, ext, 4) == 0)
		{
			Add(current->d_name);
		}
	}
	closedir(dp);
#endif

	// Sort the list of files alphabetically.
	struct {
		bool operator()(const char* a, const char* b) const {
			return strcmp(a, b) < 0;
		}
	} cmp;
	std::sort(files.begin(), files.end(), cmp);
}