/*
Copyright (c) 2011 Ole Kniemeyer, MAXON, www.maxon.net

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <string.h>

#include "b3ConvexHullComputer.h"
#include "Bullet3Common/b3AlignedObjectArray.h"
#include "Bullet3Common/b3MinMax.h"
#include "Bullet3Common/b3Vector3.h"

#ifdef __GNUC__
#include <stdint.h>
typedef int32_t btInt32_t;
typedef int64_t btInt64_t;
typedef uint32_t btUint32_t;
typedef uint64_t btUint64_t;
#elif defined(_MSC_VER)
typedef __int32 btInt32_t;
typedef __int64 btInt64_t;
typedef unsigned __int32 btUint32_t;
typedef unsigned __int64 btUint64_t;
#else
typedef int btInt32_t;
typedef long long int btInt64_t;
typedef unsigned int btUint32_t;
typedef unsigned long long int btUint64_t;
#endif

//The definition of USE_X86_64_ASM is moved into the build system. You can enable it manually by commenting out the following lines
//#if (defined(__GNUC__) && defined(__x86_64__) && !defined(__ICL))  // || (defined(__ICL) && defined(_M_X64))   bug in Intel compiler, disable inline assembly
//	#define USE_X86_64_ASM
//#endif

//#define DEBUG_CONVEX_HULL
//#define SHOW_ITERATIONS

#if defined(DEBUG_CONVEX_HULL) || defined(SHOW_ITERATIONS)
#include <stdio.h>
#endif

// Convex hull implementation based on Preparata and Hong
// Ole Kniemeyer, MAXON Computer GmbH
class b3ConvexHullInternal
{
public:
	class Point64
	{
	public:
		btInt64_t x;
		btInt64_t y;
		btInt64_t z;

		Point64(btInt64_t x, btInt64_t y, btInt64_t z) : x(x), y(y), z(z)
		{
		}

		bool isZero()
		{
			return (x == 0) && (y == 0) && (z == 0);
		}

		btInt64_t dot(const Point64& b) const
		{
			return x * b.x + y * b.y + z * b.z;
		}
	};

	class Point32
	{
	public:
		btInt32_t x;
		btInt32_t y;
		btInt32_t z;
		int index;

		Point32()
		{
		}

		Point32(btInt32_t x, btInt32_t y, btInt32_t z) : x(x), y(y), z(z), index(-1)
		{
		}

		bool operator==(const Point32& b) const
		{
			return (x == b.x) && (y == b.y) && (z == b.z);
		}

		bool operator!=(const Point32& b) const
		{
			return (x != b.x) || (y != b.y) || (z != b.z);
		}

		bool isZero()
		{
			return (x == 0) && (y == 0) && (z == 0);
		}

		Point64 cross(const Point32& b) const
		{
			return Point64(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
		}

		Point64 cross(const Point64& b) const
		{
			return Point64(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
		}

		btInt64_t dot(const Point32& b) const
		{
			return x * b.x + y * b.y + z * b.z;
		}

		btInt64_t dot(const Point64& b) const
		{
			return x * b.x + y * b.y + z * b.z;
		}

		Point32 operator+(const Point32& b) const
		{
			return Point32(x + b.x, y + b.y, z + b.z);
		}

		Point32 operator-(const Point32& b) const
		{
			return Point32(x - b.x, y - b.y, z - b.z);
		}
	};

	class Int128
	{
	public:
		btUint64_t low;
		btUint64_t high;

		Int128()
		{
		}

		Int128(btUint64_t low, btUint64_t high) : low(low), high(high)
		{
		}

		Int128(btUint64_t low) : low(low), high(0)
		{
		}

		Int128(btInt64_t value) : low(value), high((value >= 0) ? 0 : (btUint64_t)-1LL)
		{
		}

		static Int128 mul(btInt64_t a, btInt64_t b);

		static Int128 mul(btUint64_t a, btUint64_t b);

		Int128 operator-() const
		{
			return Int128((btUint64_t) - (btInt64_t)low, ~high + (low == 0));
		}

		Int128 operator+(const Int128& b) const
		{
#ifdef USE_X86_64_ASM
			Int128 result;
			__asm__(
				"addq %[bl], %[rl]\n\t"
				"adcq %[bh], %[rh]\n\t"
				: [rl] "=r"(result.low), [rh] "=r"(result.high)
				: "0"(low), "1"(high), [bl] "g"(b.low), [bh] "g"(b.high)
				: "cc");
			return result;
#else
			btUint64_t lo = low + b.low;
			return Int128(lo, high + b.high + (lo < low));
#endif
		}

		Int128 operator-(const Int128& b) const
		{
#ifdef USE_X86_64_ASM
			Int128 result;
			__asm__(
				"subq %[bl], %[rl]\n\t"
				"sbbq %[bh], %[rh]\n\t"
				: [rl] "=r"(result.low), [rh] "=r"(result.high)
				: "0"(low), "1"(high), [bl] "g"(b.low), [bh] "g"(b.high)
				: "cc");
			return result;
#else
			return *this + -b;
#endif
		}

		Int128& operator+=(const Int128& b)
		{
#ifdef USE_X86_64_ASM
			__asm__(
				"addq %[bl], %[rl]\n\t"
				"adcq %[bh], %[rh]\n\t"
				: [rl] "=r"(low), [rh] "=r"(high)
				: "0"(low), "1"(high), [bl] "g"(b.low), [bh] "g"(b.high)
				: "cc");
#else
			btUint64_t lo = low + b.low;
			if (lo < low)
			{
				++high;
			}
			low = lo;
			high += b.high;
#endif
			return *this;
		}

		Int128& operator++()
		{
			if (++low == 0)
			{
				++high;
			}
			return *this;
		}

		Int128 operator*(btInt64_t b) const;

		b3Scalar toScalar() const
		{
			return ((btInt64_t)high >= 0) ? b3Scalar(high) * (b3Scalar(0x100000000LL) * b3Scalar(0x100000000LL)) + b3Scalar(low)
										  : -(-*this).toScalar();
		}

		int getSign() const
		{
			return ((btInt64_t)high < 0) ? -1 : (high || low) ? 1 : 0;
		}

		bool operator<(const Int128& b) const
		{
			return (high < b.high) || ((high == b.high) && (low < b.low));
		}

		int ucmp(const Int128& b) const
		{
			if (high < b.high)
			{
				return -1;
			}
			if (high > b.high)
			{
				return 1;
			}
			if (low < b.low)
			{
				return -1;
			}
			if (low > b.low)
			{
				return 1;
			}
			return 0;
		}
	};

	class Rational64
	{
	private:
		btUint64_t m_numerator;
		btUint64_t m_denominator;
		int sign;

	public:
		Rational64(btInt64_t numerator, btInt64_t denominator)
		{
			if (numerator > 0)
			{
				sign = 1;
				m_numerator = (btUint64_t)numerator;
			}
			else if (numerator < 0)
			{
				sign = -1;
				m_numerator = (btUint64_t)-numerator;
			}
			else
			{
				sign = 0;
				m_numerator = 0;
			}
			if (denominator > 0)
			{
				m_denominator = (btUint64_t)denominator;
			}
			else if (denominator < 0)
			{
				sign = -sign;
				m_denominator = (btUint64_t)-denominator;
			}
			else
			{
				m_denominator = 0;
			}
		}

		bool isNegativeInfinity() const
		{
			return (sign < 0) && (m_denominator == 0);
		}

		bool isNaN() const
		{
			return (sign == 0) && (m_denominator == 0);
		}

		int compare(const Rational64& b) const;

		b3Scalar toScalar() const
		{
			return sign * ((m_denominator == 0) ? B3_INFINITY : (b3Scalar)m_numerator / m_denominator);
		}
	};

	class Rational128
	{
	private:
		Int128 numerator;
		Int128 denominator;
		int sign;
		bool isInt64;

	public:
		Rational128(btInt64_t value)
		{
			if (value > 0)
			{
				sign = 1;
				this->numerator = value;
			}
			else if (value < 0)
			{
				sign = -1;
				this->numerator = -value;
			}
			else
			{
				sign = 0;
				this->numerator = (btUint64_t)0;
			}
			this->denominator = (btUint64_t)1;
			isInt64 = true;
		}

		Rational128(const Int128& numerator, const Int128& denominator)
		{
			sign = numerator.getSign();
			if (sign >= 0)
			{
				this->numerator = numerator;
			}
			else
			{
				this->numerator = -numerator;
			}
			int dsign = denominator.getSign();
			if (dsign >= 0)
			{
				this->denominator = denominator;
			}
			else
			{
				sign = -sign;
				this->denominator = -denominator;
			}
			isInt64 = false;
		}

		int compare(const Rational128& b) const;

		int compare(btInt64_t b) const;

		b3Scalar toScalar() const
		{
			return sign * ((denominator.getSign() == 0) ? B3_INFINITY : numerator.toScalar() / denominator.toScalar());
		}
	};

	class PointR128
	{
	public:
		Int128 x;
		Int128 y;
		Int128 z;
		Int128 denominator;

		PointR128()
		{
		}

		PointR128(Int128 x, Int128 y, Int128 z, Int128 denominator) : x(x), y(y), z(z), denominator(denominator)
		{
		}

		b3Scalar xvalue() const
		{
			return x.toScalar() / denominator.toScalar();
		}

		b3Scalar yvalue() const
		{
			return y.toScalar() / denominator.toScalar();
		}

		b3Scalar zvalue() const
		{
			return z.toScalar() / denominator.toScalar();
		}
	};

	class Edge;
	class Face;

	class Vertex
	{
	public:
		Vertex* next;
		Vertex* prev;
		Edge* edges;
		Face* firstNearbyFace;
		Face* lastNearbyFace;
		PointR128 point128;
		Point32 point;
		int copy;

		Vertex() : next(NULL), prev(NULL), edges(NULL), firstNearbyFace(NULL), lastNearbyFace(NULL), copy(-1)
		{
		}

#ifdef DEBUG_CONVEX_HULL
		void print()
		{
			b3Printf("V%d (%d, %d, %d)", point.index, point.x, point.y, point.z);
		}

		void printGraph();
#endif

		Point32 operator-(const Vertex& b) const
		{
			return point - b.point;
		}

		Rational128 dot(const Point64& b) const
		{
			return (point.index >= 0) ? Rational128(point.dot(b))
									  : Rational128(point128.x * b.x + point128.y * b.y + point128.z * b.z, point128.denominator);
		}

		b3Scalar xvalue() const
		{
			return (point.index >= 0) ? b3Scalar(point.x) : point128.xvalue();
		}

		b3Scalar yvalue() const
		{
			return (point.index >= 0) ? b3Scalar(point.y) : point128.yvalue();
		}

		b3Scalar zvalue() const
		{
			return (point.index >= 0) ? b3Scalar(point.z) : point128.zvalue();
		}

		void receiveNearbyFaces(Vertex* src)
		{
			if (lastNearbyFace)
			{
				lastNearbyFace->nextWithSameNearbyVertex = src->firstNearbyFace;
			}
			else
			{
				firstNearbyFace = src->firstNearbyFace;
			}
			if (src->lastNearbyFace)
			{
				lastNearbyFace = src->lastNearbyFace;
			}
			for (Face* f = src->firstNearbyFace; f; f = f->nextWithSameNearbyVertex)
			{
				b3Assert(f->nearbyVertex == src);
				f->nearbyVertex = this;
			}
			src->firstNearbyFace = NULL;
			src->lastNearbyFace = NULL;
		}
	};

	class Edge
	{
	public:
		Edge* next;
		Edge* prev;
		Edge* reverse;
		Vertex* target;
		Face* face;
		int copy;

		~Edge()
		{
			next = NULL;
			prev = NULL;
			reverse = NULL;
			target = NULL;
			face = NULL;
		}

		void link(Edge* n)
		{
			b3Assert(reverse->target == n->reverse->target);
			next = n;
			n->prev = this;
		}

#ifdef DEBUG_CONVEX_HULL
		void print()
		{
			b3Printf("E%p : %d -> %d,  n=%p p=%p   (0 %d\t%d\t%d) -> (%d %d %d)", this, reverse->target->point.index, target->point.index, next, prev,
					 reverse->target->point.x, reverse->target->point.y, reverse->target->point.z, target->point.x, target->point.y, target->point.z);
		}
#endif
	};

	class Face
	{
	public:
		Face* next;
		Vertex* nearbyVertex;
		Face* nextWithSameNearbyVertex;
		Point32 origin;
		Point32 dir0;
		Point32 dir1;

		Face() : next(NULL), nearbyVertex(NULL), nextWithSameNearbyVertex(NULL)
		{
		}

		void init(Vertex* a, Vertex* b, Vertex* c)
		{
			nearbyVertex = a;
			origin = a->point;
			dir0 = *b - *a;
			dir1 = *c - *a;
			if (a->lastNearbyFace)
			{
				a->lastNearbyFace->nextWithSameNearbyVertex = this;
			}
			else
			{
				a->firstNearbyFace = this;
			}
			a->lastNearbyFace = this;
		}

		Point64 getNormal()
		{
			return dir0.cross(dir1);
		}
	};

	template <typename UWord, typename UHWord>
	class DMul
	{
	private:
		static btUint32_t high(btUint64_t value)
		{
			return (btUint32_t)(value >> 32);
		}

		static btUint32_t low(btUint64_t value)
		{
			return (btUint32_t)value;
		}

		static btUint64_t mul(btUint32_t a, btUint32_t b)
		{
			return (btUint64_t)a * (btUint64_t)b;
		}

		static void shlHalf(btUint64_t& value)
		{
			value <<= 32;
		}

		static btUint64_t high(Int128 value)
		{
			return value.high;
		}

		static btUint64_t low(Int128 value)
		{
			return value.low;
		}

		static Int128 mul(btUint64_t a, btUint64_t b)
		{
			return Int128::mul(a, b);
		}

		static void shlHalf(Int128& value)
		{
			value.high = value.low;
			value.low = 0;
		}

	public:
		static void mul(UWord a, UWord b, UWord& resLow, UWord& resHigh)
		{
			UWord p00 = mul(low(a), low(b));
			UWord p01 = mul(low(a), high(b));
			UWord p10 = mul(high(a), low(b));
			UWord p11 = mul(high(a), high(b));
			UWord p0110 = UWord(low(p01)) + UWord(low(p10));
			p11 += high(p01);
			p11 += high(p10);
			p11 += high(p0110);
			shlHalf(p0110);
			p00 += p0110;
			if (p00 < p0110)
			{
				++p11;
			}
			resLow = p00;
			resHigh = p11;
		}
	};

private:
	class IntermediateHull
	{
	public:
		Vertex* minXy;
		Vertex* maxXy;
		Vertex* minYx;
		Vertex* maxYx;

		IntermediateHull() : minXy(NULL), maxXy(NULL), minYx(NULL), maxYx(NULL)
		{
		}

		void print();
	};

	enum Orientation
	{
		NONE,
		CLOCKWISE,
		COUNTER_CLOCKWISE
	};

	template <typename T>
	class PoolArray
	{
	private:
		T* array;
		int size;

	public:
		PoolArray<T>* next;

		PoolArray(int size) : size(size), next(NULL)
		{
			array = (T*)b3AlignedAlloc(sizeof(T) * size, 16);
		}

		~PoolArray()
		{
			b3AlignedFree(array);
		}

		T* init()
		{
			T* o = array;
			for (int i = 0; i < size; i++, o++)
			{
				o->next = (i + 1 < size) ? o + 1 : NULL;
			}
			return array;
		}
	};

	template <typename T>
	class Pool
	{
	private:
		PoolArray<T>* arrays;
		PoolArray<T>* nextArray;
		T* freeObjects;
		int arraySize;

	public:
		Pool() : arrays(NULL), nextArray(NULL), freeObjects(NULL), arraySize(256)
		{
		}

		~Pool()
		{
			while (arrays)
			{
				PoolArray<T>* p = arrays;
				arrays = p->next;
				p->~PoolArray<T>();
				b3AlignedFree(p);
			}
		}

		void reset()
		{
			nextArray = arrays;
			freeObjects = NULL;
		}

		void setArraySize(int arraySize)
		{
			this->arraySize = arraySize;
		}

		T* newObject()
		{
			T* o = freeObjects;
			if (!o)
			{
				PoolArray<T>* p = nextArray;
				if (p)
				{
					nextArray = p->next;
				}
				else
				{
					p = new (b3AlignedAlloc(sizeof(PoolArray<T>), 16)) PoolArray<T>(arraySize);
					p->next = arrays;
					arrays = p;
				}
				o = p->init();
			}
			freeObjects = o->next;
			return new (o) T();
		};

		void freeObject(T* object)
		{
			object->~T();
			object->next = freeObjects;
			freeObjects = object;
		}
	};

	b3Vector3 scaling;
	b3Vector3 center;
	Pool<Vertex> vertexPool;
	Pool<Edge> edgePool;
	Pool<Face> facePool;
	b3AlignedObjectArray<Vertex*> originalVertices;
	int mergeStamp;
	int minAxis;
	int medAxis;
	int maxAxis;
	int usedEdgePairs;
	int maxUsedEdgePairs;

	static Orientation getOrientation(const Edge* prev, const Edge* next, const Point32& s, const Point32& t);
	Edge* findMaxAngle(bool ccw, const Vertex* start, const Point32& s, const Point64& rxs, const Point64& sxrxs, Rational64& minCot);
	void findEdgeForCoplanarFaces(Vertex* c0, Vertex* c1, Edge*& e0, Edge*& e1, Vertex* stop0, Vertex* stop1);

	Edge* newEdgePair(Vertex* from, Vertex* to);

	void removeEdgePair(Edge* edge)
	{
		Edge* n = edge->next;
		Edge* r = edge->reverse;

		b3Assert(edge->target && r->target);

		if (n != edge)
		{
			n->prev = edge->prev;
			edge->prev->next = n;
			r->target->edges = n;
		}
		else
		{
			r->target->edges = NULL;
		}

		n = r->next;

		if (n != r)
		{
			n->prev = r->prev;
			r->prev->next = n;
			edge->target->edges = n;
		}
		else
		{
			edge->target->edges = NULL;
		}

		edgePool.freeObject(edge);
		edgePool.freeObject(r);
		usedEdgePairs--;
	}

	void computeInternal(int start, int end, IntermediateHull& result);

	bool mergeProjection(IntermediateHull& h0, IntermediateHull& h1, Vertex*& c0, Vertex*& c1);

	void merge(IntermediateHull& h0, IntermediateHull& h1);

	b3Vector3 toBtVector(const Point32& v);

	b3Vector3 getBtNormal(Face* face);

	bool shiftFace(Face* face, b3Scalar amount, b3AlignedObjectArray<Vertex*> stack);

public:
	Vertex* vertexList;

	void compute(const void* coords, bool doubleCoords, int stride, int count);

	b3Vector3 getCoordinates(const Vertex* v);

	b3Scalar shrink(b3Scalar amount, b3Scalar clampAmount);
};

b3ConvexHullInternal::Int128 b3ConvexHullInternal::Int128::operator*(btInt64_t b) const
{
	bool negative = (btInt64_t)high < 0;
	Int128 a = negative ? -*this : *this;
	if (b < 0)
	{
		negative = !negative;
		b = -b;
	}
	Int128 result = mul(a.low, (btUint64_t)b);
	result.high += a.high * (btUint64_t)b;
	return negative ? -result : result;
}

b3ConvexHullInternal::Int128 b3ConvexHullInternal::Int128::mul(btInt64_t a, btInt64_t b)
{
	Int128 result;

#ifdef USE_X86_64_ASM
	__asm__("imulq %[b]"
			: "=a"(result.low), "=d"(result.high)
			: "0"(a), [b] "r"(b)
			: "cc");
	return result;

#else
	bool negative = a < 0;
	if (negative)
	{
		a = -a;
	}
	if (b < 0)
	{
		negative = !negative;
		b = -b;
	}
	DMul<btUint64_t, btUint32_t>::mul((btUint64_t)a, (btUint64_t)b, result.low, result.high);
	return negative ? -result : result;
#endif
}

b3ConvexHullInternal::Int128 b3ConvexHullInternal::Int128::mul(btUint64_t a, btUint64_t b)
{
	Int128 result;

#ifdef USE_X86_64_ASM
	__asm__("mulq %[b]"
			: "=a"(result.low), "=d"(result.high)
			: "0"(a), [b] "r"(b)
			: "cc");

#else
	DMul<btUint64_t, btUint32_t>::mul(a, b, result.low, result.high);
#endif

	return result;
}

int b3ConvexHullInternal::Rational64::compare(const Rational64& b) const
{
	if (sign != b.sign)
	{
		return sign - b.sign;
	}
	else if (sign == 0)
	{
		return 0;
	}

	//	return (numerator * b.denominator > b.numerator * denominator) ? sign : (numerator * b.denominator < b.numerator * denominator) ? -sign : 0;

#ifdef USE_X86_64_ASM

	int result;
	btInt64_t tmp;
	btInt64_t dummy;
	__asm__(
		"mulq %[bn]\n\t"
		"movq %%rax, %[tmp]\n\t"
		"movq %%rdx, %%rbx\n\t"
		"movq %[tn], %%rax\n\t"
		"mulq %[bd]\n\t"
		"subq %[tmp], %%rax\n\t"
		"sbbq %%rbx, %%rdx\n\t"  // rdx:rax contains 128-bit-difference "numerator*b.denominator - b.numerator*denominator"
		"setnsb %%bh\n\t"        // bh=1 if difference is non-negative, bh=0 otherwise
		"orq %%rdx, %%rax\n\t"
		"setnzb %%bl\n\t"      // bl=1 if difference if non-zero, bl=0 if it is zero
		"decb %%bh\n\t"        // now bx=0x0000 if difference is zero, 0xff01 if it is negative, 0x0001 if it is positive (i.e., same sign as difference)
		"shll $16, %%ebx\n\t"  // ebx has same sign as difference
		: "=&b"(result), [tmp] "=&r"(tmp), "=a"(dummy)
		: "a"(denominator), [bn] "g"(b.numerator), [tn] "g"(numerator), [bd] "g"(b.denominator)
		: "%rdx", "cc");
	return result ? result ^ sign  // if sign is +1, only bit 0 of result is inverted, which does not change the sign of result (and cannot result in zero)
								   // if sign is -1, all bits of result are inverted, which changes the sign of result (and again cannot result in zero)
				  : 0;

#else

	return sign * Int128::mul(m_numerator, b.m_denominator).ucmp(Int128::mul(m_denominator, b.m_numerator));

#endif
}

int b3ConvexHullInternal::Rational128::compare(const Rational128& b) const
{
	if (sign != b.sign)
	{
		return sign - b.sign;
	}
	else if (sign == 0)
	{
		return 0;
	}
	if (isInt64)
	{
		return -b.compare(sign * (btInt64_t)numerator.low);
	}

	Int128 nbdLow, nbdHigh, dbnLow, dbnHigh;
	DMul<Int128, btUint64_t>::mul(numerator, b.denominator, nbdLow, nbdHigh);
	DMul<Int128, btUint64_t>::mul(denominator, b.numerator, dbnLow, dbnHigh);

	int cmp = nbdHigh.ucmp(dbnHigh);
	if (cmp)
	{
		return cmp * sign;
	}
	return nbdLow.ucmp(dbnLow) * sign;
}

int b3ConvexHullInternal::Rational128::compare(btInt64_t b) const
{
	if (isInt64)
	{
		btInt64_t a = sign * (btInt64_t)numerator.low;
		return (a > b) ? 1 : (a < b) ? -1 : 0;
	}
	if (b > 0)
	{
		if (sign <= 0)
		{
			return -1;
		}
	}
	else if (b < 0)
	{
		if (sign >= 0)
		{
			return 1;
		}
		b = -b;
	}
	else
	{
		return sign;
	}

	return numerator.ucmp(denominator * b) * sign;
}

b3ConvexHullInternal::Edge* b3ConvexHullInternal::newEdgePair(Vertex* from, Vertex* to)
{
	b3Assert(from && to);
	Edge* e = edgePool.newObject();
	Edge* r = edgePool.newObject();
	e->reverse = r;
	r->reverse = e;
	e->copy = mergeStamp;
	r->copy = mergeStamp;
	e->target = to;
	r->target = from;
	e->face = NULL;
	r->face = NULL;
	usedEdgePairs++;
	if (usedEdgePairs > maxUsedEdgePairs)
	{
		maxUsedEdgePairs = usedEdgePairs;
	}
	return e;
}

bool b3ConvexHullInternal::mergeProjection(IntermediateHull& h0, IntermediateHull& h1, Vertex*& c0, Vertex*& c1)
{
	Vertex* v0 = h0.maxYx;
	Vertex* v1 = h1.minYx;
	if ((v0->point.x == v1->point.x) && (v0->point.y == v1->point.y))
	{
		b3Assert(v0->point.z < v1->point.z);
		Vertex* v1p = v1->prev;
		if (v1p == v1)
		{
			c0 = v0;
			if (v1->edges)
			{
				b3Assert(v1->edges->next == v1->edges);
				v1 = v1->edges->target;
				b3Assert(v1->edges->next == v1->edges);
			}
			c1 = v1;
			return false;
		}
		Vertex* v1n = v1->next;
		v1p->next = v1n;
		v1n->prev = v1p;
		if (v1 == h1.minXy)
		{
			if ((v1n->point.x < v1p->point.x) || ((v1n->point.x == v1p->point.x) && (v1n->point.y < v1p->point.y)))
			{
				h1.minXy = v1n;
			}
			else
			{
				h1.minXy = v1p;
			}
		}
		if (v1 == h1.maxXy)
		{
			if ((v1n->point.x > v1p->point.x) || ((v1n->point.x == v1p->point.x) && (v1n->point.y > v1p->point.y)))
			{
				h1.maxXy = v1n;
			}
			else
			{
				h1.maxXy = v1p;
			}
		}
	}

	v0 = h0.maxXy;
	v1 = h1.maxXy;
	Vertex* v00 = NULL;
	Vertex* v10 = NULL;
	btInt32_t sign = 1;

	for (int side = 0; side <= 1; side++)
	{
		btInt32_t dx = (v1->point.x - v0->point.x) * sign;
		if (dx > 0)
		{
			while (true)
			{
				btInt32_t dy = v1->point.y - v0->point.y;

				Vertex* w0 = side ? v0->next : v0->prev;
				if (w0 != v0)
				{
					btInt32_t dx0 = (w0->point.x - v0->point.x) * sign;
					btInt32_t dy0 = w0->point.y - v0->point.y;
					if ((dy0 <= 0) && ((dx0 == 0) || ((dx0 < 0) && (dy0 * dx <= dy * dx0))))
					{
						v0 = w0;
						dx = (v1->point.x - v0->point.x) * sign;
						continue;
					}
				}

				Vertex* w1 = side ? v1->next : v1->prev;
				if (w1 != v1)
				{
					btInt32_t dx1 = (w1->point.x - v1->point.x) * sign;
					btInt32_t dy1 = w1->point.y - v1->point.y;
					btInt32_t dxn = (w1->point.x - v0->point.x) * sign;
					if ((dxn > 0) && (dy1 < 0) && ((dx1 == 0) || ((dx1 < 0) && (dy1 * dx < dy * dx1))))
					{
						v1 = w1;
						dx = dxn;
						continue;
					}
				}

				break;
			}
		}
		else if (dx < 0)
		{
			while (true)
			{
				btInt32_t dy = v1->point.y - v0->point.y;

				Vertex* w1 = side ? v1->prev : v1->next;
				if (w1 != v1)
				{
					btInt32_t dx1 = (w1->point.x - v1->point.x) * sign;
					btInt32_t dy1 = w1->point.y - v1->point.y;
					if ((dy1 >= 0) && ((dx1 == 0) || ((dx1 < 0) && (dy1 * dx <= dy * dx1))))
					{
						v1 = w1;
						dx = (v1->point.x - v0->point.x) * sign;
						continue;
					}
				}

				Vertex* w0 = side ? v0->prev : v0->next;
				if (w0 != v0)
				{
					btInt32_t dx0 = (w0->point.x - v0->point.x) * sign;
					btInt32_t dy0 = w0->point.y - v0->point.y;
					btInt32_t dxn = (v1->point.x - w0->point.x) * sign;
					if ((dxn < 0) && (dy0 > 0) && ((dx0 == 0) || ((dx0 < 0) && (dy0 * dx < dy * dx0))))
					{
						v0 = w0;
						dx = dxn;
						continue;
					}
				}

				break;
			}
		}
		else
		{
			btInt32_t x = v0->point.x;
			btInt32_t y0 = v0->point.y;
			Vertex* w0 = v0;
			Vertex* t;
			while (((t = side ? w0->next : w0->prev) != v0) && (t->point.x == x) && (t->point.y <= y0))
			{
				w0 = t;
				y0 = t->point.y;
			}
			v0 = w0;

			btInt32_t y1 = v1->point.y;
			Vertex* w1 = v1;
			while (((t = side ? w1->prev : w1->next) != v1) && (t->point.x == x) && (t->point.y >= y1))
			{
				w1 = t;
				y1 = t->point.y;
			}
			v1 = w1;
		}

		if (side == 0)
		{
			v00 = v0;
			v10 = v1;

			v0 = h0.minXy;
			v1 = h1.minXy;
			sign = -1;
		}
	}

	v0->prev = v1;
	v1->next = v0;

	v00->next = v10;
	v10->prev = v00;

	if (h1.minXy->point.x < h0.minXy->point.x)
	{
		h0.minXy = h1.minXy;
	}
	if (h1.maxXy->point.x >= h0.maxXy->point.x)
	{
		h0.maxXy = h1.maxXy;
	}

	h0.maxYx = h1.maxYx;

	c0 = v00;
	c1 = v10;

	return true;
}

void b3ConvexHullInternal::computeInternal(int start, int end, IntermediateHull& result)
{
	int n = end - start;
	switch (n)
	{
		case 0:
			result.minXy = NULL;
			result.maxXy = NULL;
			result.minYx = NULL;
			result.maxYx = NULL;
			return;
		case 2:
		{
			Vertex* v = originalVertices[start];
			Vertex* w = v + 1;
			if (v->point != w->point)
			{
				btInt32_t dx = v->point.x - w->point.x;
				btInt32_t dy = v->point.y - w->point.y;

				if ((dx == 0) && (dy == 0))
				{
					if (v->point.z > w->point.z)
					{
						Vertex* t = w;
						w = v;
						v = t;
					}
					b3Assert(v->point.z < w->point.z);
					v->next = v;
					v->prev = v;
					result.minXy = v;
					result.maxXy = v;
					result.minYx = v;
					result.maxYx = v;
				}
				else
				{
					v->next = w;
					v->prev = w;
					w->next = v;
					w->prev = v;

					if ((dx < 0) || ((dx == 0) && (dy < 0)))
					{
						result.minXy = v;
						result.maxXy = w;
					}
					else
					{
						result.minXy = w;
						result.maxXy = v;
					}

					if ((dy < 0) || ((dy == 0) && (dx < 0)))
					{
						result.minYx = v;
						result.maxYx = w;
					}
					else
					{
						result.minYx = w;
						result.maxYx = v;
					}
				}

				Edge* e = newEdgePair(v, w);
				e->link(e);
				v->edges = e;

				e = e->reverse;
				e->link(e);
				w->edges = e;

				return;
			}
		}
		// lint -fallthrough
		case 1:
		{
			Vertex* v = originalVertices[start];
			v->edges = NULL;
			v->next = v;
			v->prev = v;

			result.minXy = v;
			result.maxXy = v;
			result.minYx = v;
			result.maxYx = v;

			return;
		}
	}

	int split0 = start + n / 2;
	Point32 p = originalVertices[split0 - 1]->point;
	int split1 = split0;
	while ((split1 < end) && (originalVertices[split1]->point == p))
	{
		split1++;
	}
	computeInternal(start, split0, result);
	IntermediateHull hull1;
	computeInternal(split1, end, hull1);
#ifdef DEBUG_CONVEX_HULL
	b3Printf("\n\nMerge\n");
	result.print();
	hull1.print();
#endif
	merge(result, hull1);
#ifdef DEBUG_CONVEX_HULL
	b3Printf("\n  Result\n");
	result.print();
#endif
}

#ifdef DEBUG_CONVEX_HULL
void b3ConvexHullInternal::IntermediateHull::print()
{
	b3Printf("    Hull\n");
	for (Vertex* v = minXy; v;)
	{
		b3Printf("      ");
		v->print();
		if (v == maxXy)
		{
			b3Printf(" maxXy");
		}
		if (v == minYx)
		{
			b3Printf(" minYx");
		}
		if (v == maxYx)
		{
			b3Printf(" maxYx");
		}
		if (v->next->prev != v)
		{
			b3Printf(" Inconsistency");
		}
		b3Printf("\n");
		v = v->next;
		if (v == minXy)
		{
			break;
		}
	}
	if (minXy)
	{
		minXy->copy = (minXy->copy == -1) ? -2 : -1;
		minXy->printGraph();
	}
}

void b3ConvexHullInternal::Vertex::printGraph()
{
	print();
	b3Printf("\nEdges\n");
	Edge* e = edges;
	if (e)
	{
		do
		{
			e->print();
			b3Printf("\n");
			e = e->next;
		} while (e != edges);
		do
		{
			Vertex* v = e->target;
			if (v->copy != copy)
			{
				v->copy = copy;
				v->printGraph();
			}
			e = e->next;
		} while (e != edges);
	}
}
#endif

b3ConvexHullInternal::Orientation b3ConvexHullInternal::getOrientation(const Edge* prev, const Edge* next, const Point32& s, const Point32& t)
{
	b3Assert(prev->reverse->target == next->reverse->target);
	if (prev->next == next)
	{
		if (prev->prev == next)
		{
			Point64 n = t.cross(s);
			Point64 m = (*prev->target - *next->reverse->target).cross(*next->target - *next->reverse->target);
			b3Assert(!m.isZero());
			btInt64_t dot = n.dot(m);
			b3Assert(dot != 0);
			return (dot > 0) ? COUNTER_CLOCKWISE : CLOCKWISE;
		}
		return COUNTER_CLOCKWISE;
	}
	else if (prev->prev == next)
	{
		return CLOCKWISE;
	}
	else
	{
		return NONE;
	}
}

b3ConvexHullInternal::Edge* b3ConvexHullInternal::findMaxAngle(bool ccw, const Vertex* start, const Point32& s, const Point64& rxs, const Point64& sxrxs, Rational64& minCot)
{
	Edge* minEdge = NULL;

#ifdef DEBUG_CONVEX_HULL
	b3Printf("find max edge for %d\n", start->point.index);
#endif
	Edge* e = start->edges;
	if (e)
	{
		do
		{
			if (e->copy > mergeStamp)
			{
				Point32 t = *e->target - *start;
				Rational64 cot(t.dot(sxrxs), t.dot(rxs));
#ifdef DEBUG_CONVEX_HULL
				b3Printf("      Angle is %f (%d) for ", (float)b3Atan(cot.toScalar()), (int)cot.isNaN());
				e->print();
#endif
				if (cot.isNaN())
				{
					b3Assert(ccw ? (t.dot(s) < 0) : (t.dot(s) > 0));
				}
				else
				{
					int cmp;
					if (minEdge == NULL)
					{
						minCot = cot;
						minEdge = e;
					}
					else if ((cmp = cot.compare(minCot)) < 0)
					{
						minCot = cot;
						minEdge = e;
					}
					else if ((cmp == 0) && (ccw == (getOrientation(minEdge, e, s, t) == COUNTER_CLOCKWISE)))
					{
						minEdge = e;
					}
				}
#ifdef DEBUG_CONVEX_HULL
				b3Printf("\n");
#endif
			}
			e = e->next;
		} while (e != start->edges);
	}
	return minEdge;
}

void b3ConvexHullInternal::findEdgeForCoplanarFaces(Vertex* c0, Vertex* c1, Edge*& e0, Edge*& e1, Vertex* stop0, Vertex* stop1)
{
	Edge* start0 = e0;
	Edge* start1 = e1;
	Point32 et0 = start0 ? start0->target->point : c0->point;
	Point32 et1 = start1 ? start1->target->point : c1->point;
	Point32 s = c1->point - c0->point;
	Point64 normal = ((start0 ? start0 : start1)->target->point - c0->point).cross(s);
	btInt64_t dist = c0->point.dot(normal);
	b3Assert(!start1 || (start1->target->point.dot(normal) == dist));
	Point64 perp = s.cross(normal);
	b3Assert(!perp.isZero());

#ifdef DEBUG_CONVEX_HULL
	b3Printf("   Advancing %d %d  (%p %p, %d %d)\n", c0->point.index, c1->point.index, start0, start1, start0 ? start0->target->point.index : -1, start1 ? start1->target->point.index : -1);
#endif

	btInt64_t maxDot0 = et0.dot(perp);
	if (e0)
	{
		while (e0->target != stop0)
		{
			Edge* e = e0->reverse->prev;
			if (e->target->point.dot(normal) < dist)
			{
				break;
			}
			b3Assert(e->target->point.dot(normal) == dist);
			if (e->copy == mergeStamp)
			{
				break;
			}
			btInt64_t dot = e->target->point.dot(perp);
			if (dot <= maxDot0)
			{
				break;
			}
			maxDot0 = dot;
			e0 = e;
			et0 = e->target->point;
		}
	}

	btInt64_t maxDot1 = et1.dot(perp);
	if (e1)
	{
		while (e1->target != stop1)
		{
			Edge* e = e1->reverse->next;
			if (e->target->point.dot(normal) < dist)
			{
				break;
			}
			b3Assert(e->target->point.dot(normal) == dist);
			if (e->copy == mergeStamp)
			{
				break;
			}
			btInt64_t dot = e->target->point.dot(perp);
			if (dot <= maxDot1)
			{
				break;
			}
			maxDot1 = dot;
			e1 = e;
			et1 = e->target->point;
		}
	}

#ifdef DEBUG_CONVEX_HULL
	b3Printf("   Starting at %d %d\n", et0.index, et1.index);
#endif

	btInt64_t dx = maxDot1 - maxDot0;
	if (dx > 0)
	{
		while (true)
		{
			btInt64_t dy = (et1 - et0).dot(s);

			if (e0 && (e0->target != stop0))
			{
				Edge* f0 = e0->next->reverse;
				if (f0->copy > mergeStamp)
				{
					btInt64_t dx0 = (f0->target->point - et0).dot(perp);
					btInt64_t dy0 = (f0->target->point - et0).dot(s);
					if ((dx0 == 0) ? (dy0 < 0) : ((dx0 < 0) && (Rational64(dy0, dx0).compare(Rational64(dy, dx)) >= 0)))
					{
						et0 = f0->target->point;
						dx = (et1 - et0).dot(perp);
						e0 = (e0 == start0) ? NULL : f0;
						continue;
					}
				}
			}

			if (e1 && (e1->target != stop1))
			{
				Edge* f1 = e1->reverse->next;
				if (f1->copy > mergeStamp)
				{
					Point32 d1 = f1->target->point - et1;
					if (d1.dot(normal) == 0)
					{
						btInt64_t dx1 = d1.dot(perp);
						btInt64_t dy1 = d1.dot(s);
						btInt64_t dxn = (f1->target->point - et0).dot(perp);
						if ((dxn > 0) && ((dx1 == 0) ? (dy1 < 0) : ((dx1 < 0) && (Rational64(dy1, dx1).compare(Rational64(dy, dx)) > 0))))
						{
							e1 = f1;
							et1 = e1->target->point;
							dx = dxn;
							continue;
						}
					}
					else
					{
						b3Assert((e1 == start1) && (d1.dot(normal) < 0));
					}
				}
			}

			break;
		}
	}
	else if (dx < 0)
	{
		while (true)
		{
			btInt64_t dy = (et1 - et0).dot(s);

			if (e1 && (e1->target != stop1))
			{
				Edge* f1 = e1->prev->reverse;
				if (f1->copy > mergeStamp)
				{
					btInt64_t dx1 = (f1->target->point - et1).dot(perp);
					btInt64_t dy1 = (f1->target->point - et1).dot(s);
					if ((dx1 == 0) ? (dy1 > 0) : ((dx1 < 0) && (Rational64(dy1, dx1).compare(Rational64(dy, dx)) <= 0)))
					{
						et1 = f1->target->point;
						dx = (et1 - et0).dot(perp);
						e1 = (e1 == start1) ? NULL : f1;
						continue;
					}
				}
			}

			if (e0 && (e0->target != stop0))
			{
				Edge* f0 = e0->reverse->prev;
				if (f0->copy > mergeStamp)
				{
					Point32 d0 = f0->target->point - et0;
					if (d0.dot(normal) == 0)
					{
						btInt64_t dx0 = d0.dot(perp);
						btInt64_t dy0 = d0.dot(s);
						btInt64_t dxn = (et1 - f0->target->point).dot(perp);
						if ((dxn < 0) && ((dx0 == 0) ? (dy0 > 0) : ((dx0 < 0) && (Rational64(dy0, dx0).compare(Rational64(dy, dx)) < 0))))
						{
							e0 = f0;
							et0 = e0->target->point;
							dx = dxn;
							continue;
						}
					}
					else
					{
						b3Assert((e0 == start0) && (d0.dot(normal) < 0));
					}
				}
			}

			break;
		}
	}
#ifdef DEBUG_CONVEX_HULL
	b3Printf("   Advanced edges to %d %d\n", et0.index, et1.index);
#endif
}

void b3ConvexHullInternal::merge(IntermediateHull& h0, IntermediateHull& h1)
{
	if (!h1.maxXy)
	{
		return;
	}
	if (!h0.maxXy)
	{
		h0 = h1;
		return;
	}

	mergeStamp--;

	Vertex* c0 = NULL;
	Edge* toPrev0 = NULL;
	Edge* firstNew0 = NULL;
	Edge* pendingHead0 = NULL;
	Edge* pendingTail0 = NULL;
	Vertex* c1 = NULL;
	Edge* toPrev1 = NULL;
	Edge* firstNew1 = NULL;
	Edge* pendingHead1 = NULL;
	Edge* pendingTail1 = NULL;
	Point32 prevPoint;

	if (mergeProjection(h0, h1, c0, c1))
	{
		Point32 s = *c1 - *c0;
		Point64 normal = Point32(0, 0, -1).cross(s);
		Point64 t = s.cross(normal);
		b3Assert(!t.isZero());

		Edge* e = c0->edges;
		Edge* start0 = NULL;
		if (e)
		{
			do
			{
				btInt64_t dot = (*e->target - *c0).dot(normal);
				b3Assert(dot <= 0);
				if ((dot == 0) && ((*e->target - *c0).dot(t) > 0))
				{
					if (!start0 || (getOrientation(start0, e, s, Point32(0, 0, -1)) == CLOCKWISE))
					{
						start0 = e;
					}
				}
				e = e->next;
			} while (e != c0->edges);
		}

		e = c1->edges;
		Edge* start1 = NULL;
		if (e)
		{
			do
			{
				btInt64_t dot = (*e->target - *c1).dot(normal);
				b3Assert(dot <= 0);
				if ((dot == 0) && ((*e->target - *c1).dot(t) > 0))
				{
					if (!start1 || (getOrientation(start1, e, s, Point32(0, 0, -1)) == COUNTER_CLOCKWISE))
					{
						start1 = e;
					}
				}
				e = e->next;
			} while (e != c1->edges);
		}

		if (start0 || start1)
		{
			findEdgeForCoplanarFaces(c0, c1, start0, start1, NULL, NULL);
			if (start0)
			{
				c0 = start0->target;
			}
			if (start1)
			{
				c1 = start1->target;
			}
		}

		prevPoint = c1->point;
		prevPoint.z++;
	}
	else
	{
		prevPoint = c1->point;
		prevPoint.x++;
	}

	Vertex* first0 = c0;
	Vertex* first1 = c1;
	bool firstRun = true;

	while (true)
	{
		Point32 s = *c1 - *c0;
		Point32 r = prevPoint - c0->point;
		Point64 rxs = r.cross(s);
		Point64 sxrxs = s.cross(rxs);

#ifdef DEBUG_CONVEX_HULL
		b3Printf("\n  Checking %d %d\n", c0->point.index, c1->point.index);
#endif
		Rational64 minCot0(0, 0);
		Edge* min0 = findMaxAngle(false, c0, s, rxs, sxrxs, minCot0);
		Rational64 minCot1(0, 0);
		Edge* min1 = findMaxAngle(true, c1, s, rxs, sxrxs, minCot1);
		if (!min0 && !min1)
		{
			Edge* e = newEdgePair(c0, c1);
			e->link(e);
			c0->edges = e;

			e = e->reverse;
			e->link(e);
			c1->edges = e;
			return;
		}
		else
		{
			int cmp = !min0 ? 1 : !min1 ? -1 : minCot0.compare(minCot1);
#ifdef DEBUG_CONVEX_HULL
			b3Printf("    -> Result %d\n", cmp);
#endif
			if (firstRun || ((cmp >= 0) ? !minCot1.isNegativeInfinity() : !minCot0.isNegativeInfinity()))
			{
				Edge* e = newEdgePair(c0, c1);
				if (pendingTail0)
				{
					pendingTail0->prev = e;
				}
				else
				{
					pendingHead0 = e;
				}
				e->next = pendingTail0;
				pendingTail0 = e;

				e = e->reverse;
				if (pendingTail1)
				{
					pendingTail1->next = e;
				}
				else
				{
					pendingHead1 = e;
				}
				e->prev = pendingTail1;
				pendingTail1 = e;
			}

			Edge* e0 = min0;
			Edge* e1 = min1;

#ifdef DEBUG_CONVEX_HULL
			b3Printf("   Found min edges to %d %d\n", e0 ? e0->target->point.index : -1, e1 ? e1->target->point.index : -1);
#endif

			if (cmp == 0)
			{
				findEdgeForCoplanarFaces(c0, c1, e0, e1, NULL, NULL);
			}

			if ((cmp >= 0) && e1)
			{
				if (toPrev1)
				{
					for (Edge *e = toPrev1->next, *n = NULL; e != min1; e = n)
					{
						n = e->next;
						removeEdgePair(e);
					}
				}

				if (pendingTail1)
				{
					if (toPrev1)
					{
						toPrev1->link(pendingHead1);
					}
					else
					{
						min1->prev->link(pendingHead1);
						firstNew1 = pendingHead1;
					}
					pendingTail1->link(min1);
					pendingHead1 = NULL;
					pendingTail1 = NULL;
				}
				else if (!toPrev1)
				{
					firstNew1 = min1;
				}

				prevPoint = c1->point;
				c1 = e1->target;
				toPrev1 = e1->reverse;
			}

			if ((cmp <= 0) && e0)
			{
				if (toPrev0)
				{
					for (Edge *e = toPrev0->prev, *n = NULL; e != min0; e = n)
					{
						n = e->prev;
						removeEdgePair(e);
					}
				}

				if (pendingTail0)
				{
					if (toPrev0)
					{
						pendingHead0->link(toPrev0);
					}
					else
					{
						pendingHead0->link(min0->next);
						firstNew0 = pendingHead0;
					}
					min0->link(pendingTail0);
					pendingHead0 = NULL;
					pendingTail0 = NULL;
				}
				else if (!toPrev0)
				{
					firstNew0 = min0;
				}

				prevPoint = c0->point;
				c0 = e0->target;
				toPrev0 = e0->reverse;
			}
		}

		if ((c0 == first0) && (c1 == first1))
		{
			if (toPrev0 == NULL)
			{
				pendingHead0->link(pendingTail0);
				c0->edges = pendingTail0;
			}
			else
			{
				for (Edge *e = toPrev0->prev, *n = NULL; e != firstNew0; e = n)
				{
					n = e->prev;
					removeEdgePair(e);
				}
				if (pendingTail0)
				{
					pendingHead0->link(toPrev0);
					firstNew0->link(pendingTail0);
				}
			}

			if (toPrev1 == NULL)
			{
				pendingTail1->link(pendingHead1);
				c1->edges = pendingTail1;
			}
			else
			{
				for (Edge *e = toPrev1->next, *n = NULL; e != firstNew1; e = n)
				{
					n = e->next;
					removeEdgePair(e);
				}
				if (pendingTail1)
				{
					toPrev1->link(pendingHead1);
					pendingTail1->link(firstNew1);
				}
			}

			return;
		}

		firstRun = false;
	}
}

static bool b3PointCmp(const b3ConvexHullInternal::Point32& p, const b3ConvexHullInternal::Point32& q)
{
	return (p.y < q.y) || ((p.y == q.y) && ((p.x < q.x) || ((p.x == q.x) && (p.z < q.z))));
}

