/*****************************************************************//**
 * \file   Sphere.cpp
 * \brief  Class definition for ray-hittable sphere
 *
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#include "Sphere.h"

// Side-note: virtual keyword is not needed AND not allowed to be outside a class declaration, and thus for a virtual function defintion outside the class we also can declare it with override

bool Sphere::is_hit_by(const Ray& ray, double t_min, double t_max, HitRecord& record) const
{
	Vector3D sphere_factor = ray.origin() - center;
	double A = ray.direction().squared_length();
	double half_B = dot(ray.direction(), sphere_factor);
	double C = sphere_factor.squared_length() - radius * radius;
	double discriminant = half_B * half_B - A * C;

	if (discriminant < 0.0)		// at this stage, discriminant == 0 case isn't rejected.
	{
		return false;
	}

	double dist = std::sqrt(discriminant);
	double root = ((-half_B) - dist) / A;	// the "near" root
	if (root < t_min || t_max < root)
	{
		root = ((-half_B) + dist) / A;		// the "far" root
		if (root < t_min || t_max < root)	// in-range far root is ACCEPTED when near root is rejected
		{
			return false;
		}
	}
	// record the intersection:
	record.t = root;
	record.point = ray.at(root);
	record.set_normal(ray, (record.point - center) / radius);	// We decide to let normal be unit vector!!!
	record.material_pointer = material_pointer;

	return true;
}
