#include <math.h>
#include "vec2.h"
#include "box2.h"
#include "plane2.h"

plane_t::plane_t()
{}

plane_t::plane_t(float a, float b, float c)
	: a(a), b(b), c(c)
{}

plane_t::plane_t(vec2 normal, float distance)
{
	a = normal.x;
	b = normal.y;
	c = -distance;
}

float plane_t::operator[](int i) const
{
	return (&this->a)[i];
}

float& plane_t::operator[](int i)
{
	return (&this->a)[i];
}

void plane_t::Reverse()
{
	a = -a;
	b = -b;
	c = -c;
}

vec2 plane_t::GetNormal()
{
	return vec2(a, b);
}

float plane_t::GetDistance()
{
	return c;
}

float plane_t::Distance(vec2& p)
{
	return (a * p.x) + (b * p.y) + c;
}

int plane_t::PointOnPlaneSide(vec2& p, const float epsilon)
{
	float d = Distance(p);

	if(d > epsilon)
		return PLANE_SIDE_FRONT;

	if(d < -epsilon)
		return PLANE_SIDE_BACK;

	return PLANE_SIDE_ON;
}

int plane_t::BoxOnPlaneSide(box2 box, float epsilon)
{
	vec2	corners[2];
	float	dists[2];
	
	for (int i=0; i < 2; i++)
	{
		vec2 normal = GetNormal();
		
		if (normal[i] < 0)
		{
			corners[0][i] = box.min[i];
			corners[1][i] = box.max[i];
		}
		else
		{
			corners[1][i] = box.min[i];
			corners[0][i] = box.max[i];
		}
	}
	
	dists[0] = Distance(corners[0]);
	dists[1] = Distance(corners[1]);
	
	if(fabsf(dists[0]) < epsilon && fabsf(dists[1]) < epsilon)
		return PLANE_SIDE_ON;
	if(dists[0] >= 0.0f && dists[1] >= 0.0f)
		return PLANE_SIDE_FRONT;
	if(dists[0] <= 0.0f && dists[1] <= 0.0f)
		return PLANE_SIDE_BACK;
	
	return PLANE_SIDE_CROSS;
}

bool plane_t::IsAxial()
{
	return
		(a > 0.0f && b == 0.0f) ||
		(a == 0.0f && b > 0.0f);
}

plane_t plane_t::operator-()
{
	plane_t r = plane_t(a, b, c);
	r.Reverse();

	return r;
}

float Distance(plane_t plane, vec2 x)
{
	return plane.Distance(x);
}


