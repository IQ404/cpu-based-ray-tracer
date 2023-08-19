/*****************************************************************//**
 * \file   Renderer.h
 * \brief  The header file of the renderer for the 8599 ray tracer
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <execution>
#include <memory>	// to use std::shared_ptr
#include <glm/glm.hpp>		// to use glm vec
#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Camera.h"
#include "Ray.h"
#include "BVH.h"
#include "LightSource.h"

class Renderer
{
	// Average index of refractions:
	#define eta_Vacuum 1.0
	#define eta_Air 1.00029
	#define eta_20C_Water 1.333
	#define eta_Glass1 1.5
	#define eta_Glass2 1.6
	#define eta_Diamond 2.42

public:		// structs

	struct Settings
	{
		bool accumulating = true;
	};

public:		// methods

	//Renderer() = default;	// Defaulted default constructor: the compiler will define the implicit default constructor even if other constructors are present.
	Renderer();

	~Renderer()
	{
		delete bvh;
	}

	void ResizeViewport(uint32_t width, uint32_t height);
	void Render(const Camera& camera);

	std::shared_ptr<Walnut::Image> GetFinalImage() const
	{
		return frame_image_final;
	}

	void Reaccumulate()
	{
		frame_accumulating = 1;
	}

	Settings& GetSettings()
	{
		return settings;
	}

	[[nodiscard]] const std::vector<Whitted::Entity*>& GetEntities() const
	{
		return entities;
	}
	// See https://oopscenities.net/2022/01/16/cpp17-nodiscard-attribute/#:~:text=C%2B%2B17%20adds%20a,or%20assigned%20to%20a%20value.
	[[nodiscard]] const std::vector<std::unique_ptr<Whitted::PointLightSource>>& GetLightSources() const
	{
		return light_sources;
	}

	void Add(Whitted::Entity* entity_pointer)
	{
		entities.push_back(entity_pointer);
	}

	void Add(std::unique_ptr<Whitted::PointLightSource> light_source)
	{
		light_sources.push_back(std::move(light_source));
		/*
		Note:
			the move constructor of std::unique_ptr constructs a unique_ptr by transferring the ownership
			(and nulls the previous owner).
		*/
	}

	void GenerateBVH()
	{
		bvh = new AccelerationStructure::BVH{ entities };	// put into constructor? NO, since we may add entities before rendering but after initializing the World
	}

	Whitted::IntersectionRecord ray_BVH_intersection_record(const AccelerationStructure::Ray& ray) const
	{
		return bvh->traverse_BVH_from_root(ray);
	}

	glm::vec3 mirror_reflection_direction(const glm::vec3& incident_ray_direction, const glm::vec3& surface_normal) const
	{
		return incident_ray_direction - 2 * glm::dot(incident_ray_direction, surface_normal) * surface_normal;
		// Note: this algorithm works even when incident ray is coming from inside (surface normal is always pointing outwards).
	}

	glm::vec3 snell_refraction_direction(const glm::vec3& incident_ray_direction, const glm::vec3& surface_normal, const float& entity_refraction_index) const
		// Assume incident_ray_direction and surface_normal are unit vectors where the incident_ray_direction is toward to the
		// surface evaluation point and the surface_nornal is pointing outwards.
		// Assume we want the normal used in computation to always towards the incident ray, and want the incident vector used in
		// computation to always originate from the surface evaluation point.
		// The snell_refraction_direction we are returning is also a unit vector.
	{
		// Trial settings:
		float eta_in = eta_Vacuum;
		float eta_out = entity_refraction_index;
		glm::vec3 normal = surface_normal;
		// Trial settings correction:
		float cos_incident = Whitted::clamp_float(glm::dot(incident_ray_direction, surface_normal), -1, 1);
		if (cos_incident < 0)		// case 1: ray is coming from the outside
		{
			cos_incident = -cos_incident;
		}
		else	// case 2: ray is coming from the inside
				// Note that if cos_incident = 0, we treat the incident ray as coming from the inside, thus we return 0
		{
			std::swap(eta_in, eta_out);
			normal = -normal;
		}
		float eta_ratio = eta_in / eta_out;
		float cos_refract_squared = 1 - eta_ratio * eta_ratio * (1 - cos_incident * cos_incident);
		return (cos_refract_squared < 0) ?
			(glm::vec3{0.0f, 0.0f, 0.0f}) :
			(eta_ratio * incident_ray_direction + (eta_ratio * cos_incident - std::sqrtf(cos_refract_squared)) * normal);
		// if not total internal reflection, we have:
		//		refraction_direction = component_parallel_to_the_normal + component_perpendicular_to_the_normal
	}

	float accurate_fresnel_reflectance(const glm::vec3& incident_ray_direction, const glm::vec3& surface_normal, const float& entity_refraction_index) const
		// The parameters has the same assumptions as for snell_refraction_direction().
	{
		// Trial settings:
		float eta_in = eta_Vacuum;
		float eta_out = entity_refraction_index;
		// Trial settings correction:
		float cos_incident = Whitted::clamp_float(glm::dot(incident_ray_direction, surface_normal), -1, 1);
		if (cos_incident < 0)		// case 1: ray is coming from the outside
		{
			cos_incident = -cos_incident;
		}
		else	// case 2: ray is coming from the inside
				// Note that if cos_incident = 0 we treat the incident ray as coming from the inside, thus we return 1
		{
			std::swap(eta_in, eta_out);
		}
		float sin_refract = eta_in / eta_out * std::sqrtf(std::max(0.0f, 1 - cos_incident * cos_incident));		// Snell's law
		if (sin_refract > 1.0f)		// Total internal reflection
		{
			return 1.0f;
		}
		else	// Note: this is NOT Schlick¡¯s approximation, we DO consider polarization here
		{
			float cos_refract = std::sqrtf(std::max(0.0f, 1 - sin_refract * sin_refract));
			float R_s_sqrt = (eta_in * cos_incident - eta_out * cos_refract) / (eta_in * cos_incident + eta_out * cos_refract);
			float R_p_sqrt = (eta_in * cos_refract - eta_out * cos_incident) / (eta_in * cos_refract + eta_out * cos_incident);
			return (R_s_sqrt * R_s_sqrt + R_p_sqrt * R_p_sqrt) / 2;
			// See https://en.wikipedia.org/wiki/Fresnel_equations
		}
	}

	glm::vec3 cast_Whitted_ray(const AccelerationStructure::Ray& ray, int has_already_bounced) const;

private:	// methods

	void RayGen_Shader(uint32_t x, uint32_t y);	// mimic one of the vulkan shaders which is called to cast ray(s) for every pixel

private:	// members
	Settings settings;
	std::vector<uint32_t> rows;
	std::vector<uint32_t> columns;
	std::shared_ptr<Walnut::Image> frame_image_final;
	uint32_t* frame_data = nullptr;
	glm::vec4* temporal_accumulation_frame_data = nullptr;
	uint32_t frame_accumulating = 1;	// the index (starting from 1) of the current frame that is being accumulated into the temporal_accumulation_frame_data buffer
	const Camera* active_camera = nullptr;

	glm::vec3 sky_color{0.2f, 0.7f, 0.8f};
	int max_bounce_depth = 5;
	AccelerationStructure::BVH* bvh = nullptr;
	std::vector<Whitted::Entity*> entities;
	std::vector<std::unique_ptr<Whitted::PointLightSource>> light_sources;
};

#endif // !RENDERER_H