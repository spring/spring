#ifndef VEC2_H
#define VEC2_H


template<typename T>
class Vec2
{
public:
	Vec2() {};
	Vec2(const T nx, const T ny) : x(nx), y(ny) {};

	T x;
	T y;
};

typedef Vec2<int> int2;
typedef Vec2<float> float2;

#endif
