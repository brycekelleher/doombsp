#ifndef __VEC2_H__
#define __VEC2_H__

#define EPSILON_E4	(1e-4)
#define EPSILON_E8	(1e-8)

/*-----------------------------------------------------------------------------
	vec2
-----------------------------------------------------------------------------*/

class vec2
{
public:
	float	x;
	float	y;
			
	// constructors
	vec2();
	vec2(const float x, const float y);

	operator float*();
	float operator[](int i) const;
	float& operator[](int i);

	vec2 operator-() const;

	// functions
	void Set(const float _x, const float _y);
	void MakeZero();
	bool IsZero();
	bool IsNearlyZero();
	void Normalize();
	float Length();
	float LengthSquared();
	float Dot(const vec2& v) const;
	vec2 Perp() const;
	float* Ptr();
	const float* Ptr() const;
	
	// static constants
	static vec2 zero;
	static vec2 one;
	static vec2 float_max;
	static vec2 float_min;
};

vec2 operator+(const vec2 a, const vec2 b);
vec2 operator-(const vec2 a, const vec2 b);
vec2 operator*(const vec2 a, const vec2 b);
vec2 operator/(const vec2 a, const vec2 b);
vec2 operator*(const float s, const vec2 v);
vec2 operator*(const vec2 v, const float s);
vec2 operator/(const vec2 v, const float s);
bool operator==(const vec2 a, const vec2 b);
bool operator!=(const vec2 a, const vec2 b);
float Length(vec2 v);
float LengthSquared(vec2 v);
float Dot(const vec2 a, const vec2 b);
vec2 Skew(vec2 v);
vec2 Normalize(vec2 v);
vec2 Abs(vec2 v);
float LargestComponent(vec2 v);
int LargestComponentIndex(vec2 v);

extern vec2 vec2_zero;
extern vec2 vec2_one;
extern vec2 vec2_float_max;
extern vec2 vec2_float_min;

#endif

