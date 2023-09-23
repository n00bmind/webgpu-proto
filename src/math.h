/*
The MIT License

Copyright (c) 2017 Oscar Peñas Pariente <oscarpp80@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __MATH_H__
#define __MATH_H__ 

#if NON_UNITY_BUILD
#include <float.h>
#include <time.h>
#include <stdlib.h>
#include "intrinsics.h"
#endif

#define PI   3.141592653589f
#define PI64 3.14159265358979323846


template <typename T>
INLINE T Min( T a, T b )
{
    return a < b ? a : b;
}

template <typename T>
INLINE T Max( T a, T b )
{
    return a > b ? a : b;
}

template <typename T>
INLINE void Clamp( T* value, T min, T max )
{
    *value = Min( Max( *value, min ), max );
}

template <typename T>
INLINE T Abs( T value )
{
    return value >= 0 ? value : -value;
}

INLINE f32 Round( f32 value )
{
    return roundf( value );
}


INLINE bool
IsNan( f32 value )
{
    return value != value;
}

// NOTE Absolute epsilon comparison will be necessary when comparing against zero
// TODO Read once more and compare against https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
inline bool
AlmostEqual( f32 a, f32 b, f32 maxAbsDiff /*= 0*/ )
{
    bool result = false;

    if( maxAbsDiff == 0 )
    {
        result = Abs( a - b ) <= Max( Abs( a ), Abs( b ) ) * FLT_EPSILON;
    }
    else
    {
        result = Abs( a - b ) <= maxAbsDiff;
    }

    return result;
}

inline bool
AlmostZero( f32 v, f32 maxAbsDiff /*= 1e-6f */ )
{
    return Abs( v ) <= maxAbsDiff;
}

inline bool
GreaterOrAlmostEqual( f32 a, f32 b, f32 maxAbsDiff /*= 0*/ )
{
    return a > b || AlmostEqual( a, b, maxAbsDiff );
}

inline bool
LessOrAlmostEqual( f32 a, f32 b, f32 maxAbsDiff /*= 0*/ )
{
    return a < b || AlmostEqual( a, b, maxAbsDiff );
}

inline bool
AlmostEqual( f64 a, f64 b, f64 maxAbsDiff /*= 0*/ )
{
    bool result = false;

    if( maxAbsDiff == 0 )
    {
        result = Abs( a - b ) <= Max( Abs( a ), Abs( b ) ) * FLT_EPSILON;
    }
    else
    {
        result = Abs( a - b ) <= maxAbsDiff;
    }

    return result;
}

inline f32
Radians( f32 degrees )
{
    f32 result = degrees * PI / 180;
    return result;
}


// TODO Replace with MurmurHash3 128 (just truncate for 32/64 bit hashes) (https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp)
// (improve it with http://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html)
// TODO Add Meow Hash for hashing large blocks (also get the code and study it a little, there's gems in the open there!)
inline u32
Fletchef32( const void* buffer, int len )
{
	const u8* data = (u8*)buffer;
	u32 fletch1 = 0xFFFF;
	u32 fletch2 = 0xFFFF;

	while( data && len > 0 )
	{
		int l = (len <= 360) ? len : 360;
		len -= l;
		while( l > 0 )
		{
            fletch1 += *data++;
            fletch2 += fletch1;
            l--;
		}
		fletch1 = (fletch1 & 0xFFFF) + (fletch1 >> 16);
		fletch2 = (fletch2 & 0xFFFF) + (fletch2 >> 16);
	}
	return (fletch2 << 16) | (fletch1 & 0xFFFF);
}

inline u32
Pack01ToRGBA( f32 r, f32 g, f32 b, f32 a )
{
    u32 result = ((((u32)( Round( a * 255 ) ) & 0xFF) << 24)
                | (((u32)( Round( b * 255 ) ) & 0xFF) << 16)
                | (((u32)( Round( g * 255 ) ) & 0xFF) << 8)
                |  ((u32)( Round( r * 255 ) ) & 0xFF));
    return result;
}

inline u32
PackRGBA( u32 r, u32 g, u32 b, u32 a )
{
    u32 result = (((a & 0xFF) << 24)
                | ((b & 0xFF) << 16)
                | ((g & 0xFF) <<  8)
                |  (r & 0xFF));
    return result;
}

inline void
UnpackRGBA( u32 c, u32* r, u32* g, u32* b, u32* a = nullptr )
{
    if( a ) *a = (c >> 24);
    if( b ) *b = (c >> 16) & 0xFF;
    if( g ) *g = (c >>  8) & 0xFF;
    if( r ) *r = c & 0xFF;
}

#endif /* __MATH_H__ */
