/* Author: Tobi Vollebregt */

#ifndef UPCAST_H
#define UPCAST_H

// Defining this gives problems on 64 bit GCC 4.1.1,
// and SyncedSint64 & SyncedUint64 aren't needed at this moment anyway.
//#define UPCAST_USE_64_BIT_TYPES

namespace upcast {
	template<class T> struct UnaryUpcast {};
	template<class U, class V> struct BinaryUpcast {};

	/* To reduce size of the BinaryUpcast matrix, UnaryUpcast small types
	to (unsigned) int before attempting to BinaryUpcast them. */
	template<> struct UnaryUpcast<          bool  > { typedef   signed int  type; };
	template<> struct UnaryUpcast<   signed char  > { typedef   signed int  type; };
	template<> struct UnaryUpcast<   signed short > { typedef   signed int  type; };
	template<> struct UnaryUpcast<   signed int   > { typedef   signed int  type; };
	template<> struct UnaryUpcast<   signed long  > { typedef   signed long type; };
	template<> struct UnaryUpcast< unsigned char  > { typedef   signed int  type; };
	template<> struct UnaryUpcast< unsigned short > { typedef   signed int  type; };
	template<> struct UnaryUpcast< unsigned int   > { typedef unsigned int  type; };
	template<> struct UnaryUpcast< unsigned long  > { typedef unsigned long type; };
	template<> struct UnaryUpcast<          float > { typedef         float type; };
	template<> struct UnaryUpcast<         double > { typedef        double type; };
	template<> struct UnaryUpcast<    long double > { typedef   long double type; };

#ifdef UPCAST_USE_64_BIT_TYPES
	template<> struct UnaryUpcast<         int64_t > { typedef        int64_t type; };
	template<> struct UnaryUpcast<         uint64_t > { typedef        uint64_t type; };
#endif // UPCAST_USE_64_BIT_TYPES

#define UPCAST1(U, V, W) \
	template<> struct BinaryUpcast<U,V> { typedef W type; }
#define UPCAST2(U, V, W) \
	template<> struct BinaryUpcast<U,V> { typedef W type; }; \
	template<> struct BinaryUpcast<V,U> { typedef W type; }

	/* Note: this was checked using GCC 4 on AMD64 */
	/*           param1    +    param2    --> return type  */
	UPCAST1(   signed int  ,   signed int  ,   signed int  );
	UPCAST2(   signed int  ,   signed long ,   signed int  );
	UPCAST2(   signed int  , unsigned int  , unsigned int  );
	UPCAST2(   signed int  , unsigned long , unsigned long );
	UPCAST2(   signed int  ,         float ,         float );
	UPCAST2(   signed int  ,        double ,        double );
	UPCAST2(   signed int  ,   long double ,   long double );
	UPCAST1(   signed long ,   signed long ,   signed long );
	UPCAST2(   signed long , unsigned int  ,   signed long );
	UPCAST2(   signed long , unsigned long , unsigned long );
	UPCAST2(   signed long ,         float ,         float );
	UPCAST2(   signed long ,        double ,        double );
	UPCAST2(   signed long ,   long double ,   long double );
	UPCAST1( unsigned int  , unsigned int  , unsigned int  );
	UPCAST2( unsigned int  , unsigned long , unsigned long );
	UPCAST2( unsigned int  ,         float ,         float );
	UPCAST2( unsigned int  ,        double ,        double );
	UPCAST2( unsigned int  ,   long double ,   long double );
	UPCAST1( unsigned long , unsigned long , unsigned long );
	UPCAST2( unsigned long ,         float ,         float );
	UPCAST2( unsigned long ,        double ,        double );
	UPCAST2( unsigned long ,   long double ,   long double );
	UPCAST1(         float ,         float ,         float );
	UPCAST2(         float ,        double ,        double );
	UPCAST2(         float ,   long double ,   long double );
	UPCAST1(        double ,        double ,        double );
	UPCAST2(        double ,   long double ,   long double );
	UPCAST1(   long double ,   long double ,   long double );

#ifdef UPCAST_USE_64_BIT_TYPES
	UPCAST2(   signed int  ,        int64_t ,        int64_t );
	UPCAST2(   signed int  ,        uint64_t ,        uint64_t );
	UPCAST2(   signed long ,        int64_t ,        int64_t );
	UPCAST2(   signed long ,        uint64_t ,        uint64_t );
	UPCAST2( unsigned int  ,        int64_t ,        int64_t );
	UPCAST2( unsigned int  ,        uint64_t ,        uint64_t );
	UPCAST2( unsigned long ,        int64_t ,        int64_t );
	UPCAST2( unsigned long ,        uint64_t ,        uint64_t );
	UPCAST1(        int64_t ,        int64_t ,        int64_t );
	UPCAST2(        int64_t ,        uint64_t ,        uint64_t );
	UPCAST2(        int64_t ,         float ,         float );
	UPCAST2(        int64_t ,        double ,        double );
	UPCAST2(        int64_t ,   long double ,   long double );
	UPCAST1(        uint64_t ,        uint64_t ,        uint64_t );
	UPCAST2(        uint64_t ,         float ,         float );
	UPCAST2(        uint64_t ,        double ,        double );
	UPCAST2(        uint64_t ,   long double ,   long double );
#endif // UPCAST_USE_64_BIT_TYPES

#undef UPCAST1
#undef UPCAST2

	template<class U, class V> struct Upcast {
		typedef typename BinaryUpcast<typename UnaryUpcast<U>::type, typename UnaryUpcast<V>::type>::type type;
	};

#define UPCAST(U, V) \
	typename upcast::Upcast< U, V >::type
};

#endif // UPCAST_H
