#ifndef __PLANE_H__
#define __PLANE_H__

class vec2;
class box2;

enum
{
	PLANE_SIDE_FRONT	= 0,
	PLANE_SIDE_BACK		= 1,
	PLANE_SIDE_ON		= 2,
	PLANE_SIDE_CROSS	= 3
};

class plane_t
{
public:
	float	a;
	float	b;
	float	c;

	plane_t();
	plane_t(float a, float b, float c);
	plane_t(vec2 n, float d);

	// read from an indexed element
	float operator[](int i) const;
	float& operator[](int i);
	
	void Reverse();
	void PositionThroughPoint(vec2 p);
	vec2 GetNormal();
	float GetDistance();
	void Zero();

	float Distance(vec2& p);
	int PointOnPlaneSide(vec2& p, float epsilon);
	int BoxOnPlaneSide(box2 box, float epsilon);
	bool IsAxial();
	
	plane_t operator-();
};

float Distance(plane_t plane, vec2 x);

#endif
