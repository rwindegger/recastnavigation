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

#ifndef DETOURTYPES_H
#define DETOURTYPES_H

typedef unsigned short dtArea;
typedef unsigned int dtFlags;

/// The maximum number of user defined area ids.
/// This defaults to maximum possible based on dtArea typedef, but can be set smaller manually to safe space in dtQueryFilter etc
/// The upper two bits are reserved for bit packing the type and area in dtPoly
/// @ingroup detour
static const int DT_MAX_AREAS = (dtArea(~dtArea(0)) >> 2) + 1;

/// Represents the null area.
static const dtArea DT_NULL_AREA = 0;

/// The default area id used to indicate a walkable polygon. (See DT_MAX_AREAS)
static const dtArea DT_WALKABLE_AREA = (dtArea(~dtArea(0)) >> 2);

#endif DETOURTYPES_H