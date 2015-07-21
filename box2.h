#ifndef __BOX3_H__
#define __BOX3_H__

class vec2;

enum
{
	BOX_SIDE_X,
	BOX_SIDE_Y,
	BOX_SIDE_Z
};

enum
{
	BOX_SIDE_NEG_X,
	BOX_SIDE_POS_X,
	BOX_SIDE_NEG_Y,
	BOX_SIDE_POS_Y,
	BOX_SIDE_NEG_Z,
	BOX_SIDE_POS_Z
};

/*-----------------------------------------------------------------------------
	box2
-----------------------------------------------------------------------------*/

class box2
{
public:
	vec2	min;
	vec2	max;
	
	box2();
	~box2();
	
	void Clear();
	bool InsideOut();
	void AddPoint(vec2 p);
	void AddBox(box2 b);
	void Expand(float d);
	bool ContainsPoints(vec2 p);
	bool IntersectsBox(box2 b);
	void FromPoints(vec2 *points, int numpoints);
	vec2 Center();
	vec2 Size();
	int LargestSide();
};

#endif

