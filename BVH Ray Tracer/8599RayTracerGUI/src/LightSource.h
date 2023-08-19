/*****************************************************************//**
 * \file   Light.h
 * \brief  The representations of light sources in Whitted Style ray tracing
 * 
 * \author Xiaoyang Liu
 * \date   June 2023
 *********************************************************************/

#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H

#include "WhittedUtilities.h"
#include "VectorFloat.h"

namespace Whitted
{
	class PointLightSource
	{
	public:
		PointLightSource(const glm::vec3& light_source_origin, const glm::vec3& radiance)
			: m_light_source_origin{ light_source_origin }, m_radiance{ radiance }
		{

		}

		virtual ~PointLightSource() = default;

	public:
		glm::vec3 m_light_source_origin;
		glm::vec3 m_radiance;
	};
}

#endif // !LIGHTSOURCE_H