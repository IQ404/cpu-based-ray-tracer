/*****************************************************************//**
 * \file   color.h
 * \brief  Utility Functions for outputting ColorRGB object as pixel
 * 
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#ifndef COLOR_H
#define COLOR_H

#include "Vector3D.h"
#include <cmath>
#include <iostream>

inline int round_real_to_int(double r)
{
	if (r >= 0.0)
	{
		if (r - int(r) >= 0.5)
		{
			return int(r) + 1;
		}
		return int(r);
	}
	if (int(r) - r >= 0.5)
	{
		return int(r) - 1;
	}
	return int(r);
}

inline void write_color(std::ostream& os, ColorRGB pixel_color, int samples_per_pixel, int gamma)
{
	// Assume the rgb component values of colorRGB for each sample is in range [0.0, 1.0], and the output integer value is in range [0, 255].
	// Assume the rgb component values of colorRGB is the sum of all the samples

	/*double r = pixel_color.x() / samples_per_pixel;
	double g = pixel_color.y() / samples_per_pixel;
	double b = pixel_color.z() / samples_per_pixel;

	os << int(256 * clamp(r, 0.0, 0.999)) << ' '
		<< int(256 * clamp(g, 0.0, 0.999)) << ' '
		<< int(256 * clamp(b, 0.0, 0.999)) << '\n';*/

	// ??? Explain why the 0.999 can be necessary.
	// ??? What is the difference if static_cast<int>() is used instead of int()?

	os << round_real_to_int(255 * clamp(std::pow(pixel_color.x() / samples_per_pixel, 1.0 / gamma), 0.0, 1.0)) << ' '
		<< round_real_to_int(255 * clamp(std::pow(pixel_color.y() / samples_per_pixel, 1.0 / gamma), 0.0, 1.0)) << ' '
		<< round_real_to_int(255 * clamp(std::pow(pixel_color.z() / samples_per_pixel, 1.0 / gamma), 0.0, 1.0)) << '\n';
	
	// ??? Why in the first place we need gamma correction?
}

#endif // !COLOR_H