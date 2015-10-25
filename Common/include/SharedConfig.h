
#ifndef SHAREDCONFIG_H
#define SHAREDCONFIG_H

#define DT_POLYREF64

#define DT_USE64BITAREAFLAGS

#define DT_USE64BITAREAFLAGS

#include <stdint.h>

#ifdef DT_USE64BITAREAFLAGS
typedef uint64_t navAreaMask;
typedef uint64_t navIndexType;
#else
typedef unsigned int navAreaMask;
typedef unsigned int navIndexType;
#endif

const navAreaMask AREAFLAGS_WALK = ( (navAreaMask)1 << 0 );
const navAreaMask AREAFLAGS_CROUCH = ( (navAreaMask)1 << 1 );
const navAreaMask AREAFLAGS_SWIM = ( (navAreaMask)1 << 2 );
const navAreaMask AREAFLAGS_DOOR = ( (navAreaMask)1 << 3 );
const navAreaMask AREAFLAGS_JUMP = ( (navAreaMask)1 << 4 );
const navAreaMask AREAFLAGS_DISABLED = ( (navAreaMask)1 << 31 );

#endif
