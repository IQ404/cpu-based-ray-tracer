/*****************************************************************//**
 * \file   WhittedMaterial.h
 * \brief  The class to store material information for objects to be ray traced
 * 
 * \author Xiaoyang Liu
 * \date   July 2023
 *********************************************************************/

#ifndef WHITTED_MATERIAL_H
#define WHITTED_MATERIAL_H

#include "VectorFloat.h"

namespace Whitted
{
	enum MaterialNature
	{
		Diffuse
	};

	class WhittedMaterial
	{
	public:
		WhittedMaterial(
			MaterialNature material_nature = Whitted::MaterialNature::Diffuse,
			glm::vec3 emission = glm::vec3{ 0.0f,0.0f,0.0f },
			glm::vec3 diffuse_color = glm::vec3{ 1.0f,1.0f,1.0f }
		)
		{
			m_material_nature = material_nature;
			m_diffuse_color = diffuse_color;
			m_emission = emission;

			if (glm::length(emission) > 0.00001f)	// TODO: #define the "magic constant" using a meaningful name
			{
				emitting = true;
			}
			else
			{
				emitting = false;
			}
		}

		float PDF_at_the_sample(const glm::vec3& W_out, const glm::vec3& W_in, const glm::vec3& shading_point_normal)
		// Assume all vectors are outwards with respect to the shading point!!!
		// W_in and W_out are defined with respect to the movement of the photons!!!
		{
			// TODO: macrofacet model

			//if (glm::dot(W_in, shading_point_normal) >= (0.0f - 0.1f))	// account for the computational error during sampling
			if (true)
			{
				return 1.0f / (2.0f * PI);
			}
			return 0.0f;
		}

		glm::vec3 BRDF(const glm::vec3& W_out, const glm::vec3& W_in, const glm::vec3& shading_point_normal)
		// Assume all vectors are outwards with respect to the shading point!!!
		// W_in and W_out are defined with respect to the movement of the photons!!!
		{
			// TODO: macrofacet model

			if (glm::dot(W_in, shading_point_normal) >= 0.0f)
			{
				return diffuse_coefficient / PI;	// please see my MSc dissertation for derivation
			}
			return glm::vec3{0.0f, 0.0f, 0.0f};
		}

		glm::vec3 Sampling(const glm::vec3& W_out, const glm::vec3& shading_point_normal)
		// Assume all vectors are outwards with respect to the shading point
		// W_in and W_out are defined with respect to the movement of the photons!!!
		{
			// TODO: macrofacet model

			glm::vec3 W_in_local;
			W_in_local.z = get_random_float_0_1();	// TODO: this isn't (perfectly) uniform for hemisphere!!!
			float radius_on_xy_plane = std::sqrt(1.0f - W_in_local.z * W_in_local.z);
			float random_phi = 2.0f * PI * get_random_float_0_1();
			W_in_local.x = radius_on_xy_plane * std::cos(random_phi);
			W_in_local.y = radius_on_xy_plane * std::sin(random_phi);

			// TODO: compare the result of the ON-hemisphere model from the non-physical path tracer
			//       with the result of the above algorithm.


			// Now, we need to represent W_in using world coordinates:
			glm::vec3 X;
			glm::vec3 Y;
			if (std::fabs(shading_point_normal.x) > std::fabs(shading_point_normal.y))
			// in such case we know |shading_point_normal.x| > 0,
			// hence we won't have a zero vector in the following block
			{
				// Observe that:
				// shading_point_normal.x * shading_point_normal.z + shading_point_normal.z * (-shading_point_normal.x) = 0
				// We can compute a vector Y that is orthogonal to shading_point_normal:
				Y = glm::normalize(glm::vec3{ shading_point_normal.z, 0.0f, -(shading_point_normal.x) });
			}
			else
			// in such cases we have either |shading_point_normal.x| < |shading_point_normal.y|
			// or							|shading_point_normal.x| == |shading_point_normal.y|
			// In the former case, we know |shading_point_normal.y| > 0;
			// In the latter case, since shading_point_normal's length != 0,
			// we know either |shading_point_normal.y| > 0 or |shading_point_normal.z| > 0
			// Hence we won't have a zero vector in the following block
			{
				// Observe that:
				// shading_point_normal.y * shading_point_normal.z + shading_point_normal.z * (-shading_point_normal.y) = 0
				// We can compute a vector Y that is orthogonal to shading_point_normal:
				Y = glm::normalize(glm::vec3{ 0.0f, shading_point_normal.z, -(shading_point_normal.y) });
			}
			// We define shading_point_normal as pointing to the Z direction, hence we can get a right-handed coordinates as follows:
			X = glm::cross(Y, shading_point_normal);	// X is a unit vector as long as shading_point_normal is a unit vector

			return W_in_local.x * X + W_in_local.y * Y + W_in_local.z * shading_point_normal;
		}

		bool IsEmitting()
		{
			return emitting;
		}

		glm::vec3 GetEmission()
		{
			return m_emission;
		}

		MaterialNature GetMaterialNature()
		{
			return m_material_nature;
		}

		glm::vec3 GetDiffuseColor()
		{
			return m_diffuse_color;
		}

	public:
		MaterialNature m_material_nature;
		float refractive_index = 1.0f;
		glm::vec3 diffuse_coefficient;		// i.e. albedo
		glm::vec3 m_diffuse_color;
		float specular_size_factor;		// The larger this factor is, the SMALLER the specular size will be.
										// Because this will be used as an exponent.
		
		// TODO: make these private:
		glm::vec3 m_emission;
		bool emitting;
	};
}

#endif // !WHITTED_MATERIAL_H
