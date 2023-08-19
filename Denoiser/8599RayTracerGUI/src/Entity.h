/*****************************************************************//**
 * \file   Entity.h
 * \brief  The base class for any entity in the scene to be ray-traced in Whitted-Style
 * 
 * \author Xiaoyang Liu
 * \date   June 2023
 *********************************************************************/

#ifndef ENTITY_H
#define ENTITY_H

#include "WhittedUtilities.h"
#include "VectorFloat.h"
#include "BoundingVolume.h"
#include "IntersectionRecord.h"

namespace Whitted
{
	class Entity
	{
	public:
		Entity()
		{

		}

		virtual ~Entity()
		{

		}

		virtual float GetArea() = 0;

		virtual void Sampling(IntersectionRecord& sample, float& PDF) = 0;

		virtual bool IsEmissive() = 0;

		virtual AccelerationStructure::AABB_3D Get3DAABB() = 0;

		virtual glm::vec3 GetDiffuseColor(const glm::vec2& texture_coordinates = glm::vec2{ 0.0f,0.0f }) const = 0;

		virtual IntersectionRecord GetIntersectionRecord(AccelerationStructure::Ray ray) = 0;

		virtual void GetHitInfo(
			const glm::vec3& intersection, 
			const glm::vec3& light_direction, 
			const uint32_t& triangle_index, 
			const glm::vec2& barycentric_coordinates, 
			glm::vec3& surface_normal, 
			glm::vec2& texture_coordinates
		) const = 0;

	public:
		
		int id = -1;	// for temporal denoising

	};
}

#endif // !ENTITY_H