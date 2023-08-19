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
#include "Denoiser.h"

// Average index of refractions:
#define eta_Vacuum 1.0
#define eta_Air 1.00029
#define eta_20C_Water 1.333
#define eta_Glass1 1.5
#define eta_Glass2 1.6
#define eta_Diamond 2.42

class Renderer
{
public:		// structs

	struct Settings
	{
		bool immediate_clamping = true;

		bool disable_JointBilateralFiltering = true;
		bool using_JointBilateralFiltering_15 = false;
		bool using_JointBilateralFiltering_33 = false;
		bool using_JointBilateralFiltering_65 = false;

		
		bool disable_TemporalFiltering = true;
		
		bool using_temporal_kernel_7 = false;
		bool using_temporal_kernel_15 = false;
		bool using_temporal_kernel_33 = false;

		bool using_temporal_variance_tolerance_1 = false;
		bool using_temporal_variance_tolerance_2 = false;
		bool using_temporal_variance_tolerance_3 = false;

		bool using_temporal_current_frame_weighting_10 = false;
		bool using_temporal_current_frame_weighting_5 = false;
		bool using_temporal_current_frame_weighting_20 = false;
		bool using_temporal_current_frame_weighting_50 = false;
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

	void RestartTemporal()
	{
		denoiser.accessible_previous_frame = false;
	}

	std::shared_ptr<Walnut::Image> GetFinalImage() const
	{
		return frame_image_final;
	}

	void Reaccumulate()
	{
		//frame_accumulating = 1;
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

	void Add(Whitted::Entity* entity_pointer)
	{
		entities.push_back(entity_pointer);
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

	void SamplingAreaLight(Whitted::IntersectionRecord& sample, float& PDF) const
	{
		/*
		Currently the code below is designed to render Cornell Box only.
		Hence, we assume that there is only one (area)light source.
		*/
		for (uint32_t n = 0; n < entities.size(); n++)
		{
			if (entities[n]->IsEmissive())
			{
				// We have found the only light source
				// For Cornell Box, this should be a triangle mesh representing a rectangle which consists of 2 triangles
				entities[n]->Sampling(sample, PDF);

				break;
			}
		}
	}

private:	// methods

	glm::vec3 cast_path(const AccelerationStructure::Ray& ray, Denoising::G_Buffer& g_buffer, const int& column, const int& row) const;
	glm::vec3 shading(const Whitted::IntersectionRecord& record, const glm::vec3& W_out) const;
	void RayGen_Shader(uint32_t x, uint32_t y);	// mimic one of the vulkan shaders which is called to cast ray(s) for every pixel

private:	// members
	Settings settings;
	std::vector<uint32_t> rows;
	std::vector<uint32_t> columns;
	std::shared_ptr<Walnut::Image> frame_image_final;
	uint32_t* frame_data = nullptr;
	
	Denoising::G_Buffer g_buffer;
	Denoising::FrameBuffer<glm::vec3> spatial_filtered_frame_buffer;
	Denoising::FrameBuffer<glm::vec3> temporal_filtered_frame_buffer;
	Denoising::Denoiser denoiser;

	//glm::vec4* temporal_accumulation_frame_data = nullptr;
	//uint32_t frame_accumulating = 1;	// the index (starting from 1) of the current frame that is being accumulated into the temporal_accumulation_frame_data buffer
	const Camera* active_camera = nullptr;

	const float RR_survival_probability = 0.8;	// RR for "Russian Roulette"
	AccelerationStructure::BVH* bvh = nullptr;
	std::vector<Whitted::Entity*> entities;
};

#endif // !RENDERER_H