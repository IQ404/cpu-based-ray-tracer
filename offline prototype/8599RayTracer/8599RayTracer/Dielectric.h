/*****************************************************************//**
 * \file   Dielectric.h
 * \brief  Representation for refractive materials
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#ifndef DIELECTRIC_H
#define DIELECTRIC_H

#include "HitRecord.h"
#include "Material.h"

class Dielectric : public Material
{
	double m_refractive_index;

public:

	Dielectric(double refractive_index)
		: m_refractive_index{ refractive_index }
	{

	}

	virtual bool scatter(const Ray& incident_ray, const HitRecord& record, Vector3D& attenuation, Ray& scattered_ray) const override
	{
		// currently the material refracts all the incident rays whenever it can.

		attenuation = Vector3D{ 1.0,1.0,1.0 };	// no energy loss during refraction

		// Assume the given front face is pointing from the dielectric to AIR:
		double refraction_ratio = record.is_hitting_front_face ? (1.0 / m_refractive_index) : m_refractive_index;

		/*
		Note:
			If we (deliberately) pass a negative value into the radius, we will have is_hitting_front_face == false when the ray is hitting toward the negative-radius sphere,
			and have is_hitting_front_face == true when the ray is escaping the negative-radius sphere.
			Now, if this negative-radius sphere is put inside a normal sphere with positive radius, this negative-radius sphere will be acted as an air bubble (hollow) inside
			the normal sphere.
		*/

		Vector3D unit_incident_direction = unit_vector(incident_ray.direction());
		double cos_theta = std::fmin(dot(-unit_incident_direction, record.normal), 1.0);
		bool total_internal_reflection = ((refraction_ratio * std::sqrt(1.0 - cos_theta * cos_theta)) > 1.0);
		// Note: here std::fmin is to prevent error in floating point computation to pass a negative value into std::sqrt.

		if (total_internal_reflection || (specular_reflection_coefficient(cos_theta) > random_real_number()))
		{
			scattered_ray = Ray{ record.point, direction_of_mirror_reflection(unit_incident_direction, record.normal) };
			return true;
		}
		scattered_ray = Ray{ record.point, direction_of_Snell_refraction(unit_incident_direction, record.normal,refraction_ratio) };
		return true;
	}

private:

	double specular_reflection_coefficient(double cos_theta) const
	{
		// Schlick's approximation (reference: https://en.wikipedia.org/wiki/Schlick%27s_approximation)

		double r0 = ((1 - m_refractive_index) / (1 + m_refractive_index)) * ((1 - m_refractive_index) / (1 + m_refractive_index));
		return r0 + (1.0 - r0) * std::pow(1 - cos_theta, 5);
	}
};

#endif // !DIELECTRIC_H