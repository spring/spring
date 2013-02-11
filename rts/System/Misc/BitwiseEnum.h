/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BITWISEENUM_H
#define BITWISEENUM_H

/**
 * This template is loosely based on MiLi's bitwise_enums.h written by Daniel Gutson (FuDePAN).
 * Their implementation is licensed under GPLv3, so we had to rewrite it. Still all kudos should go to them.
 *
 * URL of the MiLi project: http://code.google.com/p/mili/
 **/


/**
 * @brief BitwiseEnum<class Enum>
 *
 * example usage:
 *  enum Colors {red, green, blue};
 *  typedef BitwiseEnum<Colors> Color;
 *  enum Vehicles {car, bus, train};
 *  void foo(Color c);
 *  Color c = Colors::red | Colors::green;
 *
 *  foo(c);                            // valid
 *  foo(Colors::red);                  // valid
 *  foo(Colors::red | Colors::green);  // valid
 *  foo(Vehicles::car);                // not valid
 *  foo(1);                            // not valid
 **/

namespace Bitwise
{

template <class Enum>
class BitwiseEnum
{
private:
	int v;

public:
	BitwiseEnum(const Enum& _v) : v(_v) {}
	BitwiseEnum() : v(0) {}

	//! fixes http://code.google.com/p/mili/issues/detail?id=40
	//operator int() const { return v; }
	operator Enum() const { return Enum(v); }


	// We use c++ default ones
	/*BitwiseEnum<Enum>& operator = (const Enum& v2)
	{
		return v = v2;
	}

	BitwiseEnum<Enum>& operator = (const BitwiseEnum<Enum>& be2)
	{
		return v = be2.v;
	}*/


	//===================================
	// BitwiseEnum<Enum> operators

	// Compound assignment operators
	BitwiseEnum<Enum>& operator |= (const BitwiseEnum<Enum>& be2)
	{
		v |= be2.v;
		return *this;
	}

	BitwiseEnum<Enum>& operator &= (const BitwiseEnum<Enum>& be2)
	{
		v &= be2.v;
		return *this;
	}

	BitwiseEnum<Enum>& operator ^= (const BitwiseEnum<Enum>& be2)
	{
		v ^= be2.v;
		return *this;
	}


	// Bitwise operators
	BitwiseEnum<Enum> operator | (const BitwiseEnum<Enum>& be2) const
	{
		BitwiseEnum<Enum> be(*this);
		be.v |= be2.v;
		return be;
	}

	BitwiseEnum<Enum> operator & (const BitwiseEnum<Enum>& be2) const
	{
		BitwiseEnum<Enum> be(*this);
		be.v &= be2.v;
		return be;
	}

	BitwiseEnum<Enum> operator ^ (const BitwiseEnum<Enum>& be2) const
	{
		BitwiseEnum<Enum> be(*this);
		be.v ^= be2.v;
		return be;
	}


	// Comparison operators
	bool operator == (const BitwiseEnum<Enum>& be2) const
	{
		return v == be2.v;
	}

	bool operator != (const BitwiseEnum<Enum>& be2) const
	{
		return v != be2.v;
	}


	//===================================
	// Enum operators

	// Compound assignment operators
	BitwiseEnum<Enum>& operator |= (const Enum& v2)
	{
		v |= v2;
		return *this;
	}

	BitwiseEnum<Enum>& operator &= (const Enum& v2)
	{
		v &= v2;
		return *this;
	}

	BitwiseEnum<Enum>& operator ^= (const Enum& v2)
	{
		v ^= v2;
		return *this;
	}


	// Bitwise operators
	BitwiseEnum<Enum> operator | (const Enum& v) const
	{
		BitwiseEnum<Enum> be(*this);
		be.v |= v;
		return be;
	}

	BitwiseEnum<Enum> operator & (const Enum& v) const
	{
		BitwiseEnum<Enum> be(*this);
		be.v &= v;
		return be;
	}

	BitwiseEnum<Enum> operator ^ (const Enum& v) const
	{
		BitwiseEnum<Enum> be(*this);
		be.v ^= v;
		return be;
	}


	// Comparison operators
	bool operator == (const Enum& v2) const
	{
		return v == v2;
	}

	bool operator != (const Enum& v2) const
	{
		return v != v2;
	}
};


template <class Enum>
inline BitwiseEnum<Enum> operator | (Enum a, Enum b)
{
	return BitwiseEnum<Enum>(a) | BitwiseEnum<Enum>(b);
}

}


/* WE DO NOT NEED THESE (I ASSUME)

template <class Enum>
inline BitwiseEnum<Enum> operator & (Enum a, Enum b)
{
	return BitwiseEnum<Enum>(a) | BitwiseEnum<Enum>(b);
}

template <class Enum>
inline BitwiseEnum<Enum> operator ^ (Enum a, Enum b)
{
	return BitwiseEnum<Enum>(a) | BitwiseEnum<Enum>(b);
}

*/

#endif // BITWISEENUM_H
