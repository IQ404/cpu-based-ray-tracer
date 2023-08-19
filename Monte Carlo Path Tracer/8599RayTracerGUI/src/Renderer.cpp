/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  The definitions of the renderer class for the 8599 ray tracer
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#include "Renderer.h"

#include "TriangleMesh.h"

namespace RTUtility
{
	uint32_t vecRGBA_to_0xABGR(const glm::vec4& colorRGBA)
	{
		uint8_t r = (uint8_t)(colorRGBA.r * 255.0f);
		uint8_t g = (uint8_t)(colorRGBA.g * 255.0f);
		uint8_t b = (uint8_t)(colorRGBA.b * 255.0f);
		uint8_t a = (uint8_t)(colorRGBA.a * 255.0f);

		return ((a << 24) | (b << 16) | (g << 8) | r);	// this automatically promotes 8 bits memory to 32 bits memory
	}
}

Renderer::Renderer()
{
	Whitted::WhittedMaterial* red = new Whitted::WhittedMaterial(Whitted::MaterialNature::Diffuse, glm::vec3{0.0f, 0.0f, 0.0f});
	red->diffuse_coefficient = glm::vec3{ 0.63f, 0.065f, 0.05f };
	Whitted::WhittedMaterial* green = new Whitted::WhittedMaterial(Whitted::MaterialNature::Diffuse, glm::vec3{0.0f, 0.0f, 0.0f});
	green->diffuse_coefficient = glm::vec3{ 0.1f, 0.5f, 0.1f };
	Whitted::WhittedMaterial* white = new Whitted::WhittedMaterial(Whitted::MaterialNature::Diffuse, glm::vec3{0.0f, 0.0f, 0.0f});
	white->diffuse_coefficient = glm::vec3{ 0.7f, 0.7f, 0.7f };
	Whitted::WhittedMaterial* light_material = new Whitted::WhittedMaterial(Whitted::MaterialNature::Diffuse, glm::vec3(47.8f, 38.6f, 31.1f));
	light_material->diffuse_coefficient = glm::vec3{ 0.7f, 0.7f, 0.7f };
	Whitted::TriangleMesh* floor = new Whitted::TriangleMesh("src/cornellbox/floor.obj", white);
	Whitted::TriangleMesh* shortbox = new Whitted::TriangleMesh("src/cornellbox/shortbox.obj", white);
	Whitted::TriangleMesh* tallbox = new Whitted::TriangleMesh("src/cornellbox/tallbox.obj", white);
	Whitted::TriangleMesh* left = new Whitted::TriangleMesh("src/cornellbox/left.obj", red);
	Whitted::TriangleMesh* right = new Whitted::TriangleMesh("src/cornellbox/right.obj", green);
	Whitted::TriangleMesh* light = new Whitted::TriangleMesh("src/cornellbox/light.obj", light_material);

	// The mesh file of the Stanford bunny is downloaded from https://graphics.stanford.edu/~mdfisher/Data/Meshes/bunny.obj
	// The mesh file of the Utah teapot is downloaded from https://graphics.stanford.edu/courses/cs148-10-summer/as3/code/as3/teapot.obj
	// The data for the mesh of the Cornell box is obtained from http://www.graphics.cornell.edu/online/box/data.html

	Add(floor);
	Add(shortbox);
	Add(tallbox);
	Add(left);
	Add(right);
	Add(light);

	// TODO: the current internal logic will result in memory leak of the mesh
	
	GenerateBVH();	// we should only generate BVH **once** here
}

void Renderer::ResizeViewport(uint32_t width, uint32_t height)
{
	if (frame_image_final)
	{
		if ((frame_image_final->GetWidth() == width) && (frame_image_final->GetHeight() == height))
		{
			return;
		}
		frame_image_final->Resize(width, height);
	}
	else
	{
		frame_image_final = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}
	delete[] frame_data;
	delete[] temporal_accumulation_frame_data;
	frame_data = new uint32_t[width * height];
	temporal_accumulation_frame_data = new glm::vec4[width * height];
	frame_accumulating = 1;

	rows.resize(height);
	for (uint32_t i = 0; i < height; i++)
	{
		rows[i] = i;
	}
	columns.resize(width);
	for (uint32_t i = 0; i < width; i++)
	{
		columns[i] = i;
	}
}

void Renderer::Render(const Camera& camera)
{
	active_camera = &camera;

	if (frame_accumulating == 1)
	{
		std::memset(temporal_accumulation_frame_data, 0, frame_image_final->GetWidth() * frame_image_final->GetHeight() * sizeof(glm::vec4));
	}

	std::for_each(std::execution::par, rows.begin(), rows.end(),
		[this](uint32_t y)
		{
			std::for_each(std::execution::par, columns.begin(), columns.end(),
			[this, y](uint32_t x)
				{
					RayGen_Shader(x, y);
				}
			);
		}
	);

	frame_image_final->SetData(frame_data);	// send the frame data to GPU

	if (settings.accumulating)
	{
		frame_accumulating++;
	}
	else
	{
		frame_accumulating = 1;
	}
}

void Renderer::RayGen_Shader(uint32_t x, uint32_t y)
{
	glm::vec3 color_rgb(0.0f);

	glm::vec4 color_rgba{ cast_path(AccelerationStructure::Ray{active_camera->Position(), Whitted::normalize(active_camera->RayDirections()[y * frame_image_final->GetWidth() + x])}), 1.0f };
	temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] += color_rgba;
	glm::vec4 final_color_RGBA = temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] / (float)frame_accumulating;
	final_color_RGBA = glm::clamp(final_color_RGBA, glm::vec4(0.0f), glm::vec4(1.0f));	// glm::clamp(value, min, max)

	frame_data[(y * frame_image_final->GetWidth()) + x] = RTUtility::vecRGBA_to_0xABGR(final_color_RGBA);
}

glm::vec3 Renderer::cast_path(const AccelerationStructure::Ray& ray) const
// We don't really need to record bounce depth as long as we are usign Russian Roulette.
{
	Whitted::IntersectionRecord record = ray_BVH_intersection_record(ray);
	if (record.has_intersection)
	{
		return shading(record, -(ray.m_direction));
		// Note that here we negate the direction because we want all the vectors to be outwards with respect to the shading point
	}
	return glm::vec3{12 / 255.0f, 20 / 255.0f, 69 / 255.0f};		// returns the color of night sky 12, 20, 69
}

glm::vec3 Renderer::shading(const Whitted::IntersectionRecord& record, const glm::vec3& W_out) const
{
	// Direct Emission:
	if (record.hitted_entity_material->IsEmitting())
	{
		/*
		Recall that this path tracer is designed specifically to render Cornell Box.
		The only emissive mesh we have in the Cornell Box is the area light.
		Here we assume we are having a "skylight".
		That is, all the lights "hit" the area light will "pass through" and leave Cornell Box.
		Hence, there is no n-bounce indirect illumination for n > 0 when shading any point on the area light.
		*/
		return record.hitted_entity_material->GetEmission();
	}

	glm::vec3 shading_point_normal = record.surface_normal;
	if (glm::dot(record.surface_normal, W_out) < 0.0f)
	{
		shading_point_normal = -(record.surface_normal);
	}
	glm::vec3 shading_point = record.location + shading_point_normal * INTERSECTION_CORRECTION;

	// Direct Illumination:
	glm::vec3 radiance_direct = glm::vec3{ 0.0f,0.0f,0.0f };
	Whitted::IntersectionRecord arealight_sample;
	float arealight_sample_PDF;
	SamplingAreaLight(arealight_sample, arealight_sample_PDF);
	glm::vec3 sample_location = arealight_sample.location;	// TODO: do we need INTERSECTION_CORRECTION here?
	glm::vec3 shading_point_to_sample = sample_location - shading_point;
	glm::vec3 W_in_light_source = glm::normalize(shading_point_to_sample);
	glm::vec3 arealight_sample_normal = arealight_sample.surface_normal;
	if (glm::dot(arealight_sample.surface_normal, -W_in_light_source) < 0.0f)
	{
		arealight_sample_normal = -(arealight_sample.surface_normal);
	}
	Whitted::IntersectionRecord potential_occlusion = ray_BVH_intersection_record(AccelerationStructure::Ray{shading_point, W_in_light_source});
	if (glm::length(shading_point_to_sample) < potential_occlusion.t + 0.01f)	// add 0.01 for intersection correction (in case the potential occluding object is the arealight itself)
		// not occluded
	{
		// compute 1 spp MCPT over the surface of the effective arealight:
		radiance_direct = arealight_sample.emission * record.hitted_entity_material->BRDF(W_out, W_in_light_source, shading_point_normal) * glm::dot(W_in_light_source, shading_point_normal) * glm::dot(-W_in_light_source, arealight_sample_normal) / (glm::dot(shading_point_to_sample, shading_point_to_sample)) / (arealight_sample_PDF);
	}

	// Indirect Illumination:
	glm::vec3 radiance_indirect = glm::vec3{ 0.0f,0.0f,0.0f };
	if (Whitted::get_random_float_0_1() < RR_survival_probability)
		// shot a ray from the shading point
	{
		glm::vec3 W_in = glm::normalize(record.hitted_entity_material->Sampling(W_out, shading_point_normal));
		float PDF = record.hitted_entity_material->PDF_at_the_sample(W_out, W_in, shading_point_normal);
		// TODO: check PDF != 0
		//if (PDF > 0.0f)
		{
			Whitted::IntersectionRecord deeper_ray_record = ray_BVH_intersection_record(AccelerationStructure::Ray{shading_point, W_in});
			if (deeper_ray_record.has_intersection && (!deeper_ray_record.hitted_entity_material->IsEmitting()))
				/*
				If the ray for indirect illumination does not hit any object, then we assume no light from the skybox.
				If it hits the arealight, then our function returns 0 so that it is mathematically correct to do the linear sum of radiance_direct and radiance_indirect.
				*/
			{
				radiance_indirect = shading(deeper_ray_record, -W_in) * record.hitted_entity_material->BRDF(W_out, W_in, shading_point_normal) * glm::dot(W_in, shading_point_normal) / PDF / RR_survival_probability;
			}
		}

	}
	return radiance_direct + radiance_indirect;
}