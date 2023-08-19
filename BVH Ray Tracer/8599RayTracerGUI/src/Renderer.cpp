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
	Whitted::TriangleMesh* mesh_pointer_1 = new Whitted::TriangleMesh("src/stanford_bunny.obj", 2, glm::vec3{-1,6.1,0});
	//Whitted::TriangleMesh* mesh_pointer = new Whitted::TriangleMesh("src/cube.obj");
	Whitted::TriangleMesh* mesh_pointer_2 = new Whitted::TriangleMesh("src/utah_teapot.obj", 1, glm::vec3{-1,3,0});
	
	// The mesh file of the Stanford bunny is downloaded from https://graphics.stanford.edu/~mdfisher/Data/Meshes/bunny.obj
	// The mesh file of the Utah teapot is downloaded from https://graphics.stanford.edu/courses/cs148-10-summer/as3/code/as3/teapot.obj

	Add(mesh_pointer_1);
	Add(mesh_pointer_2);
	// TODO: the current internal logic will result in memory leak of the mesh
	Add(std::make_unique<Whitted::PointLightSource>(glm::vec3(-20.0f, 70.0f, 20.0f), glm::vec3(1.0f)));
	Add(std::make_unique<Whitted::PointLightSource>(glm::vec3(20.0f, 70.0f, 20.0f), glm::vec3(1.0f)));
	
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

	glm::vec4 color_rgba{ cast_Whitted_ray(AccelerationStructure::Ray{active_camera->Position(), Whitted::normalize(active_camera->RayDirections()[y * frame_image_final->GetWidth() + x])}, 0), 1.0f };
	temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] += color_rgba;
	glm::vec4 final_color_RGBA = temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] / (float)frame_accumulating;
	final_color_RGBA = glm::clamp(final_color_RGBA, glm::vec4(0.0f), glm::vec4(1.0f));	// glm::clamp(value, min, max)

	frame_data[(y * frame_image_final->GetWidth()) + x] = RTUtility::vecRGBA_to_0xABGR(final_color_RGBA);
}

glm::vec3 Renderer::cast_Whitted_ray(const AccelerationStructure::Ray& ray, int has_already_bounced) const
{
	if ((has_already_bounced > max_bounce_depth) || (ray.m_direction == glm::vec3{0.0f, 0.0f, 0.0f}))	// TODO: should we check ray_direction here?
	{
		return glm::vec3{0.0f, 0.0f, 0.0f};		// no energy received
	}
	glm::vec3 ray_color = sky_color;
	Whitted::IntersectionRecord record = ray_BVH_intersection_record(ray);
	if (record.has_intersection)
	{
		glm::vec3 intersection = record.location;
		glm::vec3 normal_at_intersection = record.surface_normal;
		switch (record.hitted_entity_material->GetMaterialNature())
		{
		case Whitted::Reflective:
		{
			glm::vec3 reflected_ray_direction = Whitted::normalize(mirror_reflection_direction(ray.m_direction, normal_at_intersection));
			glm::vec3 reflected_ray_origin = (glm::dot(reflected_ray_direction, normal_at_intersection) < 0.0f) ?
				(intersection - normal_at_intersection * INTERSECTION_CORRECTION)
				:
				(intersection + normal_at_intersection * INTERSECTION_CORRECTION);
			// TODO: conductors have complex refractive index
			ray_color = cast_Whitted_ray(AccelerationStructure::Ray{reflected_ray_origin, reflected_ray_direction}, has_already_bounced + 1)*
				accurate_fresnel_reflectance(-reflected_ray_direction, normal_at_intersection, record.hitted_entity_material->refractive_index);
			break;
		}
		case Whitted::Reflective_Refractive:
		{
			glm::vec3 reflected_ray_direction = Whitted::normalize(
				mirror_reflection_direction(ray.m_direction, normal_at_intersection)
			);
			glm::vec3 reflected_ray_origin = (glm::dot(reflected_ray_direction, normal_at_intersection) < 0.0f) ?
				(intersection - normal_at_intersection * INTERSECTION_CORRECTION)
				:
				(intersection + normal_at_intersection * INTERSECTION_CORRECTION);

			glm::vec3 refracted_ray_direction = Whitted::normalize(
				snell_refraction_direction(ray.m_direction, normal_at_intersection, record.hitted_entity_material->refractive_index)
			);
			glm::vec3 refracted_ray_origin = (glm::dot(refracted_ray_direction, normal_at_intersection) < 0.0f) ?
				(intersection - normal_at_intersection * INTERSECTION_CORRECTION)
				:
				(intersection + normal_at_intersection * INTERSECTION_CORRECTION);

			glm::vec3 reflected_ray_color = cast_Whitted_ray(AccelerationStructure::Ray{reflected_ray_origin, reflected_ray_direction}, has_already_bounced + 1);
			glm::vec3 refracted_ray_color = cast_Whitted_ray(AccelerationStructure::Ray{refracted_ray_origin, refracted_ray_direction}, has_already_bounced + 1);

			float reflectance = accurate_fresnel_reflectance(ray.m_direction, normal_at_intersection, record.hitted_entity_material->refractive_index);
			// !!! Note that here we use reciprocity of light (in which I will be really happy if we can find an asymmetry to break it and thus publish a Siggraph :) ):
			ray_color = reflectance * reflected_ray_color + (1.0f - reflectance) * refracted_ray_color;
			break;
		}
		default:	// Diffuse_Glossy
			/*
			Notes:
			The path ends when hitting on any entity in the world whose material is defined to be Diffuse_Glossy and we assume all
			the lights coming from that end point can be shaded by a simplified Blinn-Phong model (by simple I mean to discard
			ambient term and the effect of energy dispersing with distance).
			*/
		{
			glm::vec3 total_radiance_diffuse{0.0f, 0.0f, 0.0f};
			glm::vec3 total_radiance_specular{0.0f, 0.0f, 0.0f};

			glm::vec3 shading_point = (glm::dot(ray.m_direction, normal_at_intersection) < 0.0f) ?
				(intersection + normal_at_intersection * INTERSECTION_CORRECTION)
				:
				(intersection - normal_at_intersection * INTERSECTION_CORRECTION);

			for (const auto& light_source : light_sources)
			{
				glm::vec3 light_source_direction = light_source->m_light_source_origin - intersection;
				float light_distance_squared = glm::dot(light_source_direction, light_source_direction);
				light_source_direction = Whitted::normalize(light_source_direction);

				Whitted::IntersectionRecord shadow_record = bvh->traverse_BVH_from_root(AccelerationStructure::Ray{shading_point, light_source_direction});
				// Note here that if we are inside the shaded object and the light source is outside the shaded object,
				// then the shaded point is always occluded by the shaded object itself.
				if ((shadow_record.has_intersection) && (shadow_record.t * shadow_record.t < light_distance_squared))	// is occluded
				{
					continue;
					// should we still have specular illumination here?
				}
				else
				{
					total_radiance_diffuse += light_source->m_radiance * std::fabsf(glm::dot(light_source_direction, normal_at_intersection));
					total_radiance_specular += std::powf(
						std::max(0.0f,
							-glm::dot(
								mirror_reflection_direction(-light_source_direction, normal_at_intersection),
								ray.m_direction)
						), record.hitted_entity_material->refractive_index
					) * light_source->m_radiance;
					// specularExponent = 0 means, every shading point where the cosine is not less/equal to 0 will have the same
					// specular color.
				}
			}

			ray_color = total_radiance_diffuse * record.hitted_entity->GetDiffuseColor() * record.hitted_entity_material->phong_diffuse
				+
				total_radiance_specular /* assume specular color == (1,1,1) */ * record.hitted_entity_material->phong_specular;

			break;
			/*
			Some questions:
			1. Once in shadow, should we still have specular illumination?
			2. What if the occluding entity is transparent (e.g. glass)?
			3. No need to consider cosine law for specular illumination?
			*/
		} // The end of the default statement
		} // The end of the switch statement
	}
	return ray_color;
}