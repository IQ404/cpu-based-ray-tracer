/*****************************************************************//**
 * \file   Diffuse.h
 * \brief  Representation for matte materials
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#ifndef DIFFUSE_H
#define DIFFUSE_H

#include "HitRecord.h"
#include "Material.h"

class Diffuse : public Material
{
	Vector3D albedo;	// for ColorRGB

public:

	Diffuse(const Vector3D& a)
		: albedo{ a }
	{

	}

	virtual bool scatter(const Ray& incident_ray, const HitRecord& record, Vector3D& attenuation, Ray& scattered_ray) const override
	{
#if DiffuseMode == 0	// IN-Sphere model
		Vector3D scattering_direction = record.normal + random_in_unit_sphere();
#elif DiffuseMode == 1	// ON-Sphere (Lambertian) model
		Vector3D scattering_direction = record.normal + random_unit_vector();	// ??? Explain why this (Lambertian) model is the most accurate one.
#elif DiffuseMode == 2	// IN-Hemisphere model
		Vector3D scattering_direction = random_in_unit_hemisphere(record.normal);	// Note: this is equivalent to ON-hemisphere
#endif // DiffuseMode

		if (scattering_direction.near_zero())
		{
			scattering_direction = record.normal;
		}
		scattered_ray = Ray{ record.point, scattering_direction };
		attenuation = albedo;

		return true;
	}
};

#endif // !DIFFUSE_H