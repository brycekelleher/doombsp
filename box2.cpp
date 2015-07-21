#include <float.h>
#include "vec2.h"
#include "box2.h"

box2::box2()
{
	Clear();
}

box2::~box2()
{}

void box2::Clear()
{
	min =  vec2::float_max;
	max = -vec2::float_max;
}

bool box2::InsideOut()
{
	return (min[0] <= max[0]) && (min[1] <= max[1]) && (min[2] <= max[2]);
}

void box2::AddPoint(vec2 p)
{
	for (int i = 0; i < 3; i++)
	{
		if (p[i] < min[i])
			min[i] = p[i];
		if (p[i] > max[i])
			max[i] = p[i];
	}
}

void box2::AddBox(box2 b)
{
	for (int i = 0; i < 2; i++)
	{
		if (b.min[i] < min[i])
			min[i] = b.min[i];
		if (b.min[i] > max[i])
			max[i] = b.max[i];
	}
}

void box2::Expand(float d)
{
	min[0]	+= d;
	min[1]	+= d;
	
	max[0]	+= d;
	max[1]	+= d;
}

bool box2::ContainsPoints(vec2 p)
{
	if ((p[0] >= min[0] && p[0] <= max[0]) &&
	    (p[1] >= min[1] && p[1] <= max[1]))
	{
		return true;
	}

	return false;
}

bool box2::IntersectsBox(box2 b)
{
	if ((b.min[0] < max[0] || b.max[0] > min[0]) &&
	    (b.min[1] < max[1] || b.max[1] > min[1]))
	{
		return true;
	}

	return false;
}

void box2::FromPoints(vec2 *points, int numpoints)
{
	for (int i = 0; i < numpoints; i++)
		AddPoint(points[i]);
}

vec2 box2::Center()
{
	return 0.5f * (min + max);
}

vec2 box2::Size()
{
	return max - min;
}

int box2::LargestSide()
{
	vec2 v = Size();
	
	return LargestComponentIndex(v);
}

