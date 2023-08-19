/*****************************************************************//**
 * \file   Camera.h
 * \brief  The class for the camera
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#ifndef CAMERA_H
#define CAMERA_H

#include "RayTracingToolbox.h"

class Camera
{
	Point3D origin;			// where camera locates
	Vector3D horizontal;	// for calculating the left-to-right offset of the endpoint on the focus plane
	Vector3D vertical;		// for calculating the bottom-to-top offset of the endpoint on the focus plane
	Point3D bottom_left;	// the bottom-left point on the focus plane
	Vector3D u;
	Vector3D v;
	Vector3D w;
	double lens_radius;

public:

	// Constructors:

	Camera(const Point3D& look_from, const Point3D& look_at, const Vector3D& up_direction, const double vertical_fov, const double aspect_ratio, const double aperture)
	// the fov should be provided in degree.
	// For the current implementation, up_direction can not be aligned on w (the inverse view direction).
	{
		lens_radius = aperture / 2.0;

		//double focal_length = 1.0;		// this is the distance from the camera to the viewport (projection plane).
											// We decided to default this to 1.0 so we don't explicitly define this variable anymore.
		double focus_distance = (look_at - look_from).length();

		double theta = degrees_to_radians(vertical_fov);
		double half_height = std::tan(theta / 2.0);
		double viewport_height = half_height * 2.0;
		double viewport_width = viewport_height * aspect_ratio;		// viewport has the same aspect ratio as the image if the pixels on the display is square shaped.
		
		w = unit_vector(look_from - look_at);
		u = unit_vector(cross(up_direction, w));
		v = cross(w, u);

		origin = look_from;
		horizontal = focus_distance * viewport_width * u;
		vertical = focus_distance * viewport_height * v;
		bottom_left = origin - focus_distance * w - (horizontal / 2.0) - (vertical / 2.0);
	}

	// Methods:

	Ray extract_ray(double horizontal_offset_factor, double vertical_offset_factor)
	{
		Point3D random_in_xy_disk = lens_radius * random_in_unit_xy_disk();
		Point3D random_in_lens = origin + random_in_xy_disk.x() * u + random_in_xy_disk.y() * v;

		return Ray{ random_in_lens, bottom_left + horizontal_offset_factor * horizontal + vertical_offset_factor * vertical - random_in_lens };
	}
};

#endif // !CAMERA_H