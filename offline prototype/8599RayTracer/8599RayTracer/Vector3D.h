/*****************************************************************//**
 * \file   Vector3D.h
 * \brief  The class of 3D vector for representing geometry in R^3 and RGB color
 *
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <iostream>
#include <cmath>		// to use std::sqrt etc.
#include <cassert>
//#define NDEBUG		// uncomment this if we don't want assertion (e.g. when we want things like inf)
#include "RayTracingToolbox.h"

class Vector3D
{
	double v[3];

public:

	// Constructors:

	Vector3D()
		: v{ 0,0,0 }
	{

	}

	Vector3D(double x, double y, double z)
		: v{ x,y,z }
	{

	}

	// Operators:

	Vector3D operator-() const					// additive inverse
	{
		return Vector3D{ -v[0], -v[1], -v[2] };
	}

	double& operator[](int i)
	{
		assert(i == 0 || i == 1 || i == 2);
		return v[i];
	}

	double operator[](int i) const
	{
		assert(i == 0 || i == 1 || i == 2);
		return v[i];
	}

	Vector3D& operator+=(const Vector3D& u)
	{
		v[0] += u.v[0];
		v[1] += u.v[1];
		v[2] += u.v[2];

		return *this;
	}

	Vector3D& operator*=(const double d)
	{
		v[0] *= d;
		v[1] *= d;
		v[2] *= d;

		return *this;
	}

	Vector3D& operator/=(const double d)
	{
		assert(d != 0.0);

		v[0] /= d;
		v[1] /= d;
		v[2] /= d;

		return *this;
	}

	// Methods:

	double x() const
	{
		return v[0];
	}

	double y() const
	{
		return v[1];
	}

	double z() const
	{
		return v[2];
	}

	double squared_length() const
	{
		return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	}

	double length() const
	{
		return std::sqrt(squared_length());
	}

	/*
	C++ side notes: static method can be called without instantiation, (and thus) it cannot depend on any non-static data
	*/

	inline static Vector3D random()
	{
		return Vector3D{ random_real_number(), random_real_number(), random_real_number() };
	}

	inline static Vector3D random(double min, double max)
	{
		return Vector3D{ random_real_number(min,max), random_real_number(min,max), random_real_number(min,max) };
	}

	bool near_zero() const
	{
		static const double minimum = 1e-8;
		return ((std::fabs(v[0]) < minimum) && (std::fabs(v[1]) < minimum) && (std::fabs(v[2]) < minimum));
		// C++ side note: fabs() is the floating point number version of abs()
	}
};

// Unitility Functions:

inline std::ostream& operator<<(std::ostream& os, const Vector3D& v)
{
	return os << v.x() << ' ' << v.y() << ' ' << v.z();
}

inline Vector3D operator+(const Vector3D& a, const Vector3D& b)
{
	return Vector3D{ a.x() + b.x(), a.y() + b.y(), a.z() + b.z() };
}

inline Vector3D operator-(const Vector3D& a, const Vector3D& b)
{
	return Vector3D{ a.x() - b.x(), a.y() - b.y(), a.z() - b.z() };
}

inline Vector3D operator*(const Vector3D& a, const Vector3D& b)			// Note that this is NOT dot or cross product!
{
	return Vector3D{ a.x() * b.x(), a.y() * b.y(), a.z() * b.z() };
}

inline Vector3D operator*(double d, const Vector3D& v)
{
	return Vector3D{ d * v.x(), d * v.y(), d * v.z() };
}

inline Vector3D operator*(const Vector3D& v, double d)
{
	return d * v;
}

inline Vector3D operator/(const Vector3D& v, double d)
{
	return (1 / d) * v;
}

inline double dot(const Vector3D& a, const Vector3D& b)
{
	return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

inline Vector3D cross(const Vector3D& a, const Vector3D& b)
{
	return Vector3D
	{
		a.y() * b.z() - a.z() * b.y(),
		a.z() * b.x() - a.x() * b.z(),
		a.x() * b.y() - a.y() * b.x()
	};
}

inline Vector3D unit_vector(const Vector3D& v)
{
	return v / v.length();
}

inline Vector3D random_in_unit_sphere()
{
	while (true)
	{
		Vector3D p = Vector3D::random(-1, 1);
		if (p.squared_length() >= 1.0)
		{
			continue;
		}
		return p;
	}
}

inline Vector3D random_unit_vector()
{
	return unit_vector(random_in_unit_sphere());
}

inline Vector3D random_in_unit_hemisphere(const Vector3D& normal)
{
	Vector3D p = random_in_unit_sphere();
	if (dot(p, normal) >= 0.0)		// should we include the == case?
	{
		return p;
	}
	return -p;
}

inline Vector3D random_in_unit_xy_disk()
{
	while (true)
	{
		Vector3D p{ random_real_number(-1.0,1.0),random_real_number(-1.0,1.0),0.0 };
		if (p.squared_length() >= 1.0)
		{
			continue;
		}
		return p;
	}
}

inline Vector3D direction_of_mirror_reflection(const Vector3D& incident_direction, const Vector3D& normal)
{
	return incident_direction - 2.0 * dot(incident_direction, normal) * normal;
}

inline Vector3D direction_of_Snell_refraction(const Vector3D& unit_incident_direction, const Vector3D& unit_incident_normal, double etaIN_over_etaOUT)
{
	Vector3D refraction_direction_tangential = etaIN_over_etaOUT * (unit_incident_direction + std::fmin(dot(-unit_incident_direction, unit_incident_normal), 1.0) * unit_incident_normal);
	Vector3D refraction_direction_normal = -std::sqrt(std::fabs(1.0 - refraction_direction_tangential.squared_length())) * unit_incident_normal;
	// Note: here std::fmin and std::fabs are to eliminate computation errors caused by floating point precision.
	return refraction_direction_tangential + refraction_direction_normal;
}

// For better code readability (as Vector3D will represent things with different physical meanings):

using Point3D = Vector3D;
using ColorRGB = Vector3D;

#endif // !VECTOR3D_H