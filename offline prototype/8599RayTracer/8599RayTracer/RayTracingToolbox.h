/*****************************************************************//**
 * \file   RayTracingToolbox.h
 * \brief  Numeric constants, utility functions and common headers for the ray tracer
 * 
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#ifndef RAYTRACINGTOOLBOX_H
#define RAYTRACINGTOOLBOX_H

#include <cmath>
#include <limits>
#include <memory>
#include <cstdlib>
//#include <ctime>
#include <cassert>
#include <random>

// Numeric Constants:

const double positive_infinity = std::numeric_limits<double>::infinity();	// (std::numeric_limits<double>::max() < std::numeric_limits<double>::infinity()) == true
const double pi = 3.141592653589793'2385;	// ??? Is the precision of a double able to store the digits after 3.141592653589793?

// Utility Functions:

inline double degrees_to_radians(double degrees)
{
	return (degrees / 180.0) * pi;
}

inline double random_real_number()	// returns value in [0,1)
// Note that we want to exclude 1
{
	// TODO: ??? How to randomize the seed? (We probably don't want to use std::time since we are aiming for a real-time application)

	/*
	Note that the value of RAND_MAX is implementation defined.
	With this in mind, the following expression is designed to prevent incorrect return value due to integer overflow and the limited precision of floating point calculation.
	2147483648 is chosen because, while it is large enough, we have (2147483647 / (2147483647 + 1.0)) < 1 evaluated to true.
	*/
	return (std::rand() % 2147483648) / ((RAND_MAX < 2147483648) ? (RAND_MAX + 1.0) : (2147483647 + 1.0));
}

//inline double random_real_number() {
//	static std::uniform_real_distribution<double> distribution(0.0, 1.0);
//	static std::mt19937 generator;
//	return distribution(generator);
//}

inline double random_real_number(double min, double max)	// returns value in [min,max)
{
	assert(min <= max);
	return min + (max - min) * random_real_number();
}

inline double fast_random()
/*
PCG
References see:
	https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
	https://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
*/
{
	// TODO: currently not implemented because it will generate the same image for the same scene.
}

inline double clamp(double r, double min, double max)
{
	assert(min <= max);
	if (r < min)
	{
		return min;
	}
	if (r > max)
	{
		return max;
	}
	return r;
}

// Common Headers:

#include "Vector3D.h"
#include "Ray.h"


#endif // !RAYTRACINGTOOLBOX_H

