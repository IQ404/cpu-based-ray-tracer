/*****************************************************************//**
 * \file   Metal.h
 * \brief  Representation for metallic materials
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#ifndef METAL_H
#define METAL_H

#include "HitRecord.h"
#include "Material.h"

class Metal : public Material
{
	Vector3D albedo;	// for ColorRGB
	double fuzziness;

public:

	Metal(const Vector3D& a, double f)
		: albedo{ a }, fuzziness{ f < 1 ? f : 1.0 }
	{

	}

	virtual bool scatter(const Ray& incident_ray, const HitRecord& record, Vector3D& attenuation, Ray& scattered_ray) const override
	{
		Vector3D reflection_direction = direction_of_mirror_reflection(unit_vector(incident_ray.direction()), record.normal);
		scattered_ray = Ray{ record.point, reflection_direction + fuzziness * random_in_unit_sphere()};
		attenuation = albedo;
		return (dot(reflection_direction, record.normal) > 0);	// absorb those directions into the surface due to fuzziness (??? Is this physically accurate?)
	}
};

#endif // !METAL_H