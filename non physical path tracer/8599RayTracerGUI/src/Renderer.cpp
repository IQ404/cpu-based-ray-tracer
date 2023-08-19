/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  The definitions of the renderer class for the 8599 ray tracer
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#include "Renderer.h"

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

void Renderer::Render(const Camera& camera, const NP_PathTracing::CompositeHittable& world)
{
	active_camera = &camera;
	active_world = &world;

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

ColorRGB Renderer::ray_color(const NP_PathTracing::Ray& ray, const NP_PathTracing::Hittable& world, const int bounce_depth)
{
	if (bounce_depth <= 0)
	{
		return ColorRGB{ 0.0f,0.0f,0.0f };		// representing no light
	}

	NP_PathTracing::HitRecord record;

	if (world.is_hit_by(ray, NP_PathTracing::ray_starting_offset, NP_PathTracing::positive_infinity, record))
	{
		NP_PathTracing::Ray scattered_ray;
		glm::vec3 attenuation;
		if (record.material_pointer->scatter(ray, record, attenuation, scattered_ray))
		{
			return attenuation * ray_color(scattered_ray, world, bounce_depth - 1);
		}
		// If not scattered, then it is totally absorbed by the material:
		return ColorRGB{ 0.0f,0.0f,0.0f };
	}

	// Not hitting anything: render the sky
	float interpolation_factor = 0.5f * (glm::normalize(ray.direction()).y + 1.0f);	// Normalized to [0,1]
	return (1.0f - interpolation_factor) * ColorRGB { 1.0f, 1.0f, 1.0f } + interpolation_factor * ColorRGB{ 0.5f, 0.7f, 1.0f };
}

void Renderer::RayGen_Shader(uint32_t x, uint32_t y)
{
	glm::vec3 color_rgb(0.0f);
	NP_PathTracing::Ray ray{active_camera->Position(), active_camera->RayDirections()[y * frame_image_final->GetWidth() + x]};
	color_rgb += ray_color(ray, *active_world, NP_PathTracing::max_bounce_depth);

	glm::vec4 color_rgba{ color_rgb,1.0f };
	temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] += color_rgba;
	glm::vec4 final_color_RGBA = temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] / (float)frame_accumulating;
	
	final_color_RGBA.r = std::pow(final_color_RGBA.r, 1.0f / NP_PathTracing::GetGamma());
	final_color_RGBA.g = std::pow(final_color_RGBA.g, 1.0f / NP_PathTracing::GetGamma());
	final_color_RGBA.b = std::pow(final_color_RGBA.b, 1.0f / NP_PathTracing::GetGamma());
	
	final_color_RGBA = glm::clamp(final_color_RGBA, glm::vec4(0.0f), glm::vec4(1.0f));	// glm::clamp(value, min, max)
	frame_data[(y * frame_image_final->GetWidth()) + x] = RTUtility::vecRGBA_to_0xABGR(final_color_RGBA);
}