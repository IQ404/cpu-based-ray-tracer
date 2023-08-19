/*****************************************************************//**
 * \file   Ray.h
 * \brief  The class representing the ray
 * 
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#ifndef RAY_H
#define RAY_H

#include "Vector3D.h"

class Ray
{
	// Our ray is encoded as the R->R^3 function P(t) = A + tB, where A is the origin of the line and B is the direction of the line.

	Point3D orig;
	Vector3D dir;

public:

	// Constructors:

	Ray()	// both origin and direction are initialized to (0,0,0), and thus the line degenerates to the point (0,0,0).
	{

	}

	Ray(const Point3D& origin, const Vector3D& direction)
		: orig{ origin }, dir{ direction }
	{

	}

	// Getters:

	Point3D origin() const
	{
		return orig;
	}

	Vector3D direction() const
	{
		return dir;
	}

	Point3D at(double t) const		// The point P(t) = A + tB, where A is the origin of the line and B is the direction of the line.
	{
		return orig + t * dir;
	}
};

#endif // !RAY_H
