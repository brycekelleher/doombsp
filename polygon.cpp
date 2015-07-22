#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "vec2.h"
#include "box2.h"
#include "plane2.h"
#include "polygon.h"

static int PointOnPlaneSide(vec2 p, plane_t plane, float epsilon)
{
	return plane.PointOnPlaneSide(p, epsilon);
}

// memory allocation
static void *Polygon_MemAllocHandler(int numbytes)
{
	return malloc(numbytes);
}

static void Polygon_MemFreeHandler(void *ptr)
{
	free(ptr);
}

static void *(*Polygon_MemAlloc)(int numbytes)	= Polygon_MemAllocHandler;
static void (*Polygon_MemFree)(void *p)		= Polygon_MemFreeHandler;

void Polygon_SetMemCallbacks(void *(*alloccallback)(int numbytes), void (*freecallback)(void *p))
{
	Polygon_MemAlloc	= alloccallback;
	Polygon_MemFree		= freecallback;
}

static int Polygon_MemSize(int numvertices)
{
	return sizeof(polygon_t) + (numvertices * sizeof(vec2));
}

polygon_t *Polygon_Alloc(int numvertices)
{
	polygon_t	*p;

	int numbytes = Polygon_MemSize(numvertices);
	p = (polygon_t*)Polygon_MemAlloc(numbytes);

	p->maxvertices	= numvertices;
	p->numvertices	= 0;
	p->vertices	= (vec2*)(p + 1);

	return p;
}

void Polygon_Free(polygon_t* p)
{
	Polygon_MemFree(p);
}

polygon_t* Polygon_Copy(polygon_t* p)
{
	polygon_t	*c;
	int		i;

	c = Polygon_Alloc(p->maxvertices);
	
	c->maxvertices = p->maxvertices;
	c->numvertices = p->numvertices;
	
	for (i = 0; i < c->numvertices; i++)
		c->vertices[i] = p->vertices[i];

	return c;
}

void Polygon_AddVertex(polygon_t *p, float x, float y)
{
	if (p->numvertices + 1 >= p->maxvertices)
		assert(0);

	p->vertices[p->numvertices].x = x;
	p->vertices[p->numvertices].y = y;
}

polygon_t *Polygon_Reverse(polygon_t* p)
{
	polygon_t	*r;

	r = (polygon_t*)Polygon_Alloc(p->maxvertices);

	for (int i = 0; i < p->numvertices; i++)
		r->vertices[(i + 1) % p->numvertices] = p->vertices[p->numvertices - 1 - i];

	return r;
}

box2 Polygon_BoundingBox(polygon_t* p)
{
	box2 box;
	
	for (int i = 0; i < p->numvertices; i++)
		box.AddPoint(p->vertices[i]);
	
	return box;
}

vec2 Polygon_Centroid(polygon_t* p)
{
	vec2 v;
	
	v = vec2_zero;
	for (int i = 0; i < p->numvertices; i++)
		v = v + p->vertices[i];

	v = (1.0f / (float)p->numvertices) * v;

	return v;
}

float Polygon_Area(polygon_t *p)
{
	float area = 0;
	for (int i = 0; i < p->numvertices; i++)
	{
		int j = (i + 1) % p->numvertices;
		
		vec2 vi = p->vertices[i];
		vec2 vj = p->vertices[j];
		
		area += (vi[0] * vj[1]) - (vj[0] * vi[1]);
	}
	
	return area;
}

void Polygon_SplitWithPlane(polygon_t *in, plane_t plane, float epsilon, polygon_t **front, polygon_t **back)
{
	int		sides[32+4];
	int		counts[3];		// FRONT, BACK, ON
	int		i, j;
	polygon_t	*f, *b;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

	// classify each point
	{
		for (i = 0; i < in->numvertices; i++)
		{
			sides[i] = PointOnPlaneSide(in->vertices[i], plane, epsilon);
			counts[sides[i]]++;
		}

		sides[i] = sides[0];
	}
	
	// all points are on the plane
	if (!counts[PLANE_SIDE_FRONT] && !counts[PLANE_SIDE_BACK])
	{
		*front = NULL;
		*back = NULL;
		return;
	}

	// all points are front side
	if (!counts[PLANE_SIDE_BACK])
	{
		*front = Polygon_Copy(in);
		*back = NULL;
		return;
	}

	// all points are back side
	if (!counts[PLANE_SIDE_FRONT])
	{
		*front = NULL;
		*back = Polygon_Copy(in);
		return;
	}

	// split the polygon
	maxpts = in->numvertices+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	*front = f = Polygon_Alloc(maxpts);
	*back = b = Polygon_Alloc(maxpts);
		
	for (i = 0; i < in->numvertices; i++)
	{
		vec2	p1, p2, mid;

		p1 = in->vertices[i];
		p2 = in->vertices[(i + 1) % in->numvertices];
		
		if (sides[i] == PLANE_SIDE_ON)
		{
			// add the point to the front polygon
			f->vertices[f->numvertices] = p1;
			f->numvertices++;

			// Add the point to the back polygon
			b->vertices[b->numvertices] = p1;
			b->numvertices++;
			
			continue;
		}
	
		if (sides[i] == PLANE_SIDE_FRONT)
		{
			// add the point to the front polygon
			f->vertices[f->numvertices] = p1;
			f->numvertices++;
		}

		if (sides[i] == PLANE_SIDE_BACK)
		{
			b->vertices[b->numvertices] = p1;
			b->numvertices++;
		}

		// if the next point doesn't straddle the plane continue
		if (sides[i+1] == PLANE_SIDE_ON || sides[i+1] == sides[i])
		{
			continue;
		}
		
		// The next point crosses the plane, so generate a split point
		
		for (j = 0; j < 2; j++)
		{
			// avoid round off error when possible
			if (plane[j] == 1)
			{
				mid[j] = -plane[2];
			}
			else if (plane[j] == -1)
			{
				mid[j] = plane[2];
			}
			else
			{
				float dist1, dist2, dot;
				
				dist1 = Distance(plane, p1);
				dist2 = Distance(plane, p2);
				dot = dist1 / (dist1 - dist2);
				mid[j] = ((1.0f - dot) * p1[j]) + (dot * p2[j]);
			}
		}
			
		f->vertices[f->numvertices] = mid;
		f->numvertices++;
		b->vertices[b->numvertices] = mid;
		b->numvertices++;
	}
	
	if (f->numvertices > maxpts || b->numvertices > maxpts)
	{
		assert(0);
	}

	if (f->numvertices > 32 || b->numvertices > 32)
	{
		assert(0);
	}
}

polygon_t *Polygon_ClipWithPlane(polygon_t *p, plane_t plane, float epsilon)
{
	polygon_t *f, *b;
	
	// split the polygon
	Polygon_SplitWithPlane(p, plane, epsilon, &f, &b);
	
	// free the original polygon
	Polygon_Free(p);
	
	// free the backside polygon if it exists
	if(b)
		Polygon_Free(b);
	
	return f;
}

// Classify where a polygon is with respect to a plane
int Polygon_OnPlaneSide(polygon_t *p, plane_t plane, float epsilon)
{
	bool	front, back;
	int	i;

	front = 0;
	back = 0;

	for (i = 0; i < p->numvertices; i++)
	{
		int side = PointOnPlaneSide(p->vertices[i], plane, epsilon);
		
		if (side == PLANE_SIDE_BACK)
		{
			if (front)
				return PLANE_SIDE_CROSS;
			back = 1;
			continue;
		}

		if (side == PLANE_SIDE_FRONT)
		{
			if (back)
				return PLANE_SIDE_CROSS;
			front = 1;
			continue;
		}
	}

	if (back)
		return PLANE_SIDE_BACK;
	if (front)
		return PLANE_SIDE_FRONT;
	
	return PLANE_SIDE_ON;
}



