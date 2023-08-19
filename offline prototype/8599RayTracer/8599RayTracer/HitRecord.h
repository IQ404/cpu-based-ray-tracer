/*****************************************************************//**
 * \file   HitRecord.h
 * \brief  The data structure used to (temporarily) store related information when a ray hits on an entity
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#ifndef HITRECORD_H
#define HITRECORD_H

#include "RayTracingToolbox.h"

class Material;

struct HitRecord		// ??? How is HitRecord useful as it does not record the information about the ray?
{
	std::shared_ptr<Material> material_pointer;
	Point3D point;
	Vector3D normal;
	double t;
	bool is_hitting_front_face;

	// Assume for any geometric entity rendered by this ray tracer, there is a defined front face (and thus a back face) and thus a front (i.e. outward) normal. (??? What about Klein bottle/ring?)
	// We choose to set the normal to always towards the ray:
	inline void set_normal(const Ray& ray, const Vector3D& outward_normal)		// Setting the normal and the front-back bool
	// ??? Isn't in-class method implicitly inlined? Can we delete the explicit inline keyword?
	{
		is_hitting_front_face = (dot(ray.direction(), outward_normal) < 0.0);	// Note: Hitting at right angle is counted as hitting from outside
		normal = is_hitting_front_face ? (outward_normal) : (-outward_normal);
	}
};

#endif // !HITRECORD_H
