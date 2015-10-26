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

#ifndef TESTCASE_H
#define TESTCASE_H

#include "DetourNavMesh.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

class TestCase
{
public:
	bool load(string filePath);
	
	inline const string& getSampleName() const { return m_sampleName; }
	inline const string& getGeomFileName() const { return m_geomFileName; }
	
	void doTests(class dtNavMesh* navmesh, class dtNavMeshQuery* navquery);
	
	void handleRender();
	bool handleRenderOverlay(double* proj, double* model, int* view);

private:
	enum TestType
	{
		TEST_PATHFIND,
		TEST_RAYCAST
	};

	struct Test
	{
		Test(TestType testType, bool expandRender)
			: type(testType)
			, radius(0)
			, includeFlags(0)
			, excludeFlags(0)
			, expand(expandRender)
			, nstraight(0)
			, npolys(0)
			, findNearestPolyTime(0)
			, findPathTime(0)
			, findStraightPathTime(0)
		{
			for (int i = 0; i < 3; ++i) {
				spos[i] = 0;
				epos[i] = 0;
				nspos[i] = 0;
				nepos[i] = 0;
			}
		}

		TestType type;
        float spos[3];
        float epos[3];
        float nspos[3];
        float nepos[3];
		float radius;
        unsigned short includeFlags;
        unsigned short excludeFlags;
		bool expand;

		vector<float> straight;
		int nstraight;
		vector<dtPolyRef> polys;
		int npolys;

		int findNearestPolyTime;
		int findPathTime;
		int findStraightPathTime;
	};

	void resetTimes();

	string m_sampleName;
	string m_geomFileName;
	vector<Test> m_tests;
};

#endif // TESTCASE_H