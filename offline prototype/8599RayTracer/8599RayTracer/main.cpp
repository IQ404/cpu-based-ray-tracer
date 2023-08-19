/*****************************************************************//**
 * \file   main.cpp
 * \brief  The renderer for 8599 ray tracer
 *
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

 /*
 Note:
	PPM image can be viewed by **Portable Anymap Viewer** on Windows.
 
 */  

// ---------------------------Control Panel---------------------------
#define Multithread 1				// 0 or 1
#define Antialiasing 1				// 0 or 1, currently only supports MSAA
#define GammaCorrection 1			// 0 or 1
#define ShadowAcneElimination 1		// 0 or 1
#define DiffuseMode 1				// 0: IN-sphere; 1: ON-sphere (Lambertian); 2: IN-hemisphere
#define DepthOfField 0				// 0 or 1
// -------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <chrono>		// for benchmark
#include <execution>	// for multi-threading

#include "RayTracingToolbox.h"

#include "color.h"
#include "CompositeHittable.h"
#include "Sphere.h"
#include "Camera.h"
#include "Diffuse.h"
#include "Metal.h"
#include "Dielectric.h"


ColorRGB ray_color(const Ray& ray, const Hittable& world, const int bounce_depth)
{
	if (bounce_depth <= 0)
	{
		return ColorRGB{ 0.0,0.0,0.0 };		// representing no light
	}

	HitRecord record;

#if ShadowAcneElimination
	double starting_at = 0.001;
	// We starts the ray a little bit forward from the ideal hitting point because, due to floating point precision, the calculated hitting point may not be exactly on the surface
	// This fixs the shadow acne, and is also benefical to performance.
#else
	double starting_at = 0.0;
#endif

	if (world.is_hit_by(ray, starting_at, positive_infinity, record))
	{
		Ray scattered_ray;
		Vector3D attenuation;
		if (record.material_pointer->scatter(ray, record, attenuation, scattered_ray))
		{
			return attenuation * ray_color(scattered_ray, world, bounce_depth - 1);
		}
		// If not scattered, then it is totally absorbed by the material:
		return ColorRGB{ 0.0,0.0,0.0 };
	}

	// Not hitting anything: render the sky
	double interpolation_factor = 0.5 * (unit_vector(ray.direction()).y() + 1.0);	// Normalized to [0,1]
	return (1.0 - interpolation_factor) * ColorRGB { 1.0, 1.0, 1.0 } + interpolation_factor * ColorRGB{ 0.5,0.7,1.0 };
}

int main()
{
	// Parameters of output image:
	const double aspect_ratio = 16.0 / 9.0;		// x/y
	const int image_width = 400;
	const int image_height = int(image_width / aspect_ratio);	// ??? use static_cast<int>()?

#if Antialiasing
	const int samples_per_pixel = 100;		// for MSAA
#else
	const int samples_per_pixel = 1;		// for MSAA
#endif // Antialiasing

	const int max_bounce_depth = 50;

#if GammaCorrection == 1
	const int gamma = 2;
#else
	const int gamma = 1;
#endif // GammaCorrection

	// Color Settings:
	const int max_color_value = 255;

	// Creating the (objects in the) world:
	CompositeHittable world;	// empty world

	auto left_object_material = std::make_shared<Diffuse>(Vector3D{0.0,0.0,1.0});
	auto right_object_material = std::make_shared<Diffuse>(Vector3D{ 1.0,0.0,0.0});
	double r = std::cos(pi / 4.0);
	world.add(std::make_shared<Sphere>(Point3D{ -r, 0.0, -1.0 }, r, left_object_material));
	world.add(std::make_shared<Sphere>(Point3D{ r, 0.0, -1.0 }, r, right_object_material));

	/*auto material_ground = std::make_shared<Diffuse>(ColorRGB(0.8, 0.8, 0.0));
	auto material_center = std::make_shared<Diffuse>(ColorRGB(0.1, 0.2, 0.5));
	auto material_left = std::make_shared<Dielectric>(1.5);
	auto material_right = std::make_shared<Metal>(ColorRGB(0.8, 0.6, 0.2), 0.0);

	world.add(std::make_shared<Sphere>(Point3D(0.0, -100.5, -1.0), 100.0, material_ground));
	world.add(std::make_shared<Sphere>(Point3D(0.0, 0.0, -1.0), 0.5, material_center));
	world.add(std::make_shared<Sphere>(Point3D(-1.0, 0.0, -1.0), 0.5, material_left));
	world.add(std::make_shared<Sphere>(Point3D(-1.0, 0.0, -1.0), -0.45, material_left));
	world.add(std::make_shared<Sphere>(Point3D(1.0, 0.0, -1.0), 0.5, material_right));*/

	// Camera:
#if DepthOfField
	const double aperture = 2.0;
#else
	const double aperture = 0.0;
#endif // DepthOfField
	/*Vector3D at = ((Vector3D{ 3,3,2 } - Vector3D{ 0,0,-1 }) / 8.0) + Vector3D{0,0,-1};
	Camera camera{ {3,3,2}, at, {0,1,0}, 20, aspect_ratio, aperture };*/
	Camera camera{ {0,0,0}, {0,0,-1}, {0,1,0}, 90, aspect_ratio, aperture };

	// Rendering (i.e. output data):
	// (Note that by using > operator in Windows Command Prompt the contents of std::cout can be redirected to a file while the contents of std::cerr remains in the terminal)
	std::cout << "P3" << '\n'								// colors are in ASCII		(??? Explain the meaning)
		<< image_width << ' ' << image_height << '\n'		// column  row
		<< max_color_value << '\n';								// value for max color

	// Preparations for multi-threading:
	std::vector<std::vector<ColorRGB>> image;
	image.resize(image_height);
	for (auto& row : image)
	{
		row.resize(image_width);
	}
	std::vector<int> rows(image_height);
	std::vector<int> columns(image_width);
	for (int i = 0; i < image_height; i++)
	{
		rows[i] = image_height - 1 - i;
	}
	for (int j = 0; j < image_width; j++)
	{
		columns[j] = j;
	}

	// benchmark
	auto start = std::chrono::high_resolution_clock::now();
	// RGB triplets: (For PPM format: each rgb triplet is rendered as a pixel, from left to right, top to bottom)
#if Multithread
	// Multi-threading:
	std::for_each(std::execution::par, rows.begin(), rows.end(),
		[&](int row)
		{
			std::for_each(std::execution::par, columns.begin(), columns.end(),
			[&](int column)
				{
#if Antialiasing
					ColorRGB pixel_color;	// (0,0,0)
					for (int s = 0; s < samples_per_pixel; s++)
					{
						double horizontal_offset_factor = (column + random_real_number()) / image_width;
						double vertical_offset_factor = (row + random_real_number()) / image_height;
						Ray ray = camera.extract_ray(horizontal_offset_factor, vertical_offset_factor);
						pixel_color += ray_color(ray, world, max_bounce_depth);
					}
					image[image_height - 1 - row][column] = pixel_color;
#else
					double horizontal_offset_factor = (column + 0.5) / image_width;
					double vertical_offset_factor = (row + 0.5) / image_height;
					Ray ray = camera.extract_ray(horizontal_offset_factor, vertical_offset_factor);
					ColorRGB pixel_color = ray_color(ray, world, max_bounce_depth);
					image[image_height - 1 - row][column] = pixel_color;
#endif // Antialiasing
				}
			);
		}
	);
#else
	// Single threading:
	for (int row = image_height - 1; row >= 0; row--)
	{
		std::cerr << '\r' << "Scanlines Remaining: " << row << ' ' << std::flush;		// ??? Why do we want std::flush here?
		// Note: \r means writing from the head of the current line
	
		for (int column = 0; column < image_width; column++)
		{
#if Antialiasing
			ColorRGB pixel_color;	// (0,0,0)
			for (int s = 0; s < samples_per_pixel; s++)
			{
				double horizontal_offset_factor = (column + random_real_number()) / image_width;
				double vertical_offset_factor = (row + random_real_number()) / image_height;
				Ray ray = camera.extract_ray(horizontal_offset_factor, vertical_offset_factor);
				pixel_color += ray_color(ray, world, max_bounce_depth);
			}
			image[image_height - 1 - row][column] = pixel_color;
#else
			double horizontal_offset_factor = (column + 0.5) / image_width;
			double vertical_offset_factor = (row + 0.5) / image_height;
			Ray ray = camera.extract_ray(horizontal_offset_factor, vertical_offset_factor);
			ColorRGB pixel_color = ray_color(ray, world, max_bounce_depth);
			image[image_height - 1 - row][column] = pixel_color;
#endif // Antialiasing
		}
	}
#endif // Multithread
	// Output the pixel data:
	for (const auto& row : image)
	{
		for (const auto& pixel_color : row)
		{
			write_color(std::cout, pixel_color, samples_per_pixel, gamma);
		}
	}
	// benchmark
	auto end = std::chrono::high_resolution_clock::now();
	std::cerr << '\n'
		<< "Done."
		<< '\n';
	// benchmark
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cerr << "\nIt took " << elapsed.count() << " milliseconds.\n";
}