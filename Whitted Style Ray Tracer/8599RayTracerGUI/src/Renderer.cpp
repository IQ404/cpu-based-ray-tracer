/*****************************************************************//**
 * \file   Renderer.cpp
 * \brief  The definitions of the renderer class for the 8599 ray tracer
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#include "Renderer.h"
//#include "WhittedRenderer.h"
#include "Sphere.h"
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
	auto diffuse_sphere = std::make_unique<Whitted::Sphere>(glm::vec3(-1, 0, -12), 2.0f);
	diffuse_sphere->material_nature = Whitted::Diffuse_Glossy;
	diffuse_sphere->diffuse_color = glm::vec3(0.6, 0.7, 0.8);
	world.Add(std::move(diffuse_sphere));

	auto glass_sphere = std::make_unique<Whitted::Sphere>(glm::vec3(0.5, -0.5, -8), 1.5f);
	glass_sphere->material_nature = Whitted::Reflective_Refractive;
	glass_sphere->refractive_index = 1.5;
	world.Add(std::move(glass_sphere));

	glm::vec3 vertices[4] = { {-5,-3,-6}, {5,-3,-6}, {5,-3,-16}, {-5,-3,-16} };
	uint32_t verticesIndices[6] = { 0, 1, 3, 1, 2, 3 };
	glm::vec2 texture_coordinates[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
	auto chessboard = std::make_unique<Whitted::TriangleMesh>(vertices, verticesIndices, 2, texture_coordinates);
	// Note: currently I don't want geometry that isn't closed (i.e. does not have an interior) to be reflective/refractive
	chessboard->material_nature = Whitted::Diffuse_Glossy;
	world.Add(std::move(chessboard));

	world.Add(std::make_unique<Whitted::PointLightSource>(glm::vec3(-20.0f, 70.0f, 20.0f), glm::vec3(0.5f)));
	world.Add(std::make_unique<Whitted::PointLightSource>(glm::vec3(30.0f, 50.0f, -12.0f), glm::vec3(0.5f)));
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

	glm::vec4 color_rgba{ Renderer::cast_Whitted_ray(active_camera->Position(), Whitted::normalize(active_camera->RayDirections()[y * frame_image_final->GetWidth() + x]), world, 0),1.0f };
	temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] += color_rgba;
	glm::vec4 final_color_RGBA = temporal_accumulation_frame_data[y * frame_image_final->GetWidth() + x] / (float)frame_accumulating;
	final_color_RGBA = glm::clamp(final_color_RGBA, glm::vec4(0.0f), glm::vec4(1.0f));	// glm::clamp(value, min, max)

	frame_data[(y * frame_image_final->GetWidth()) + x] = RTUtility::vecRGBA_to_0xABGR(final_color_RGBA);
}