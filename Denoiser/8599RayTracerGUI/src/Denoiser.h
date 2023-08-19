/*****************************************************************//**
 * \file   Denoiser.h
 * \brief  This header contains the tools for denoising the color value in a frame buffer given the data in the corresponding G-Buffers
 * 
 * \author Xiaoyang Liu
 * \date   July 2023
 *********************************************************************/

#ifndef DENOISER_H
#define DENOISER_H

#include <vector>
#include <glm/glm.hpp>
#include <execution>

namespace Denoising
{
	template <typename T>
	class FrameBuffer
	{
	public:

		FrameBuffer()
		{
			
		}

		FrameBuffer(const int& width, const int& height)
		{
			frame_width = width;
			frame_height = height;
			buffer.resize(width * height);
		}

		T operator()(const int& column, const int& row) const
		{
			return buffer[row * frame_width + column];
		}

		T& operator()(const int& column, const int& row)
		{
			return buffer[row * frame_width + column];
		}

		void Reset(const int& width, const int& height)
		{
			frame_width = width;
			frame_height = height;
			buffer.resize(width * height);
		}

	public:

		int frame_width = 0;
		int frame_height = 0;
		std::vector<T> buffer;

	};

	class G_Buffer
	{
	public:

		G_Buffer()
		{

		}

		G_Buffer(const int& width, const int& height)
		{
			pixel_world_position.Reset(width, height);
			pixel_color.Reset(width, height);
			pixel_world_surface_normal.Reset(width, height);
			contributor.Reset(width, height);
			primitive_id.Reset(width, height);
		}

		void Reset(const int& width, const int& height)
		{
			pixel_world_position.Reset(width, height);
			pixel_color.Reset(width, height);
			pixel_world_surface_normal.Reset(width, height);
			contributor.Reset(width, height);
			primitive_id.Reset(width, height);
		}

		// Data members:

		FrameBuffer<glm::vec3> pixel_world_position;
		FrameBuffer<glm::vec3> pixel_color;
		FrameBuffer<glm::vec3> pixel_world_surface_normal;
		FrameBuffer<int> contributor;	// 0 if the pixel is not a contributor: primary ray does not hit anything. Such pixels do not need to be denoised, neither should they contribute anything when they are inside filter kernel
										// C++ side-note: we don't want std::vector<bool>
		FrameBuffer<int> primitive_id;
		/* Side-note: glm::mat4{1} creates a diagonal matrix with 1s on its diagonal. */
		glm::mat4 projection_matrix{ 1.0f };
		glm::mat4 view_matrix{ 1.0f };
	};

	class Denoiser
	{
	public:

		Denoiser()
		{
			accessible_previous_frame = false;
		}

		void Resize(uint32_t width, uint32_t height)
		{
			accessible_previous_frame = false;

			frame_height = height;
			frame_width = width;

			//motion_vector.Reset(width, height);

			rows.resize(height);
			columns.resize(width);
			
			for (int i = 0; i < height; i++)
			{
				rows[i] = i;
			}
			for (int i = 0; i < width; i++)
			{
				columns[i] = i;
			}
		}

		// TODO: for JointBilateralFiltering() and TemporalFiltering(), will using std::swap rather than copying between g_buffer and filtered_frame_buffer be faster?

		void JointBilateralFiltering(G_Buffer& g_buffer, FrameBuffer<glm::vec3>& filtered_frame_buffer, const bool& immediate_clamp = true)
		{
			if (!using_JBF_filtering)
			{
				filtered_frame_buffer = g_buffer.pixel_color;
				return;
			}

			std::for_each(std::execution::par, rows.begin(), rows.end(),
				[&](int row)
				{
					std::for_each(std::execution::par, columns.begin(), columns.end(),
					[&](int column)
						{
							if (g_buffer.contributor(column, row) == 0)
							{
								filtered_frame_buffer(column, row) = g_buffer.pixel_color(column, row);
							}
							else
							{
								glm::vec3 filtered_pixel_color{0.0f, 0.0f, 0.0f};

								int kernel_left = std::max(0, column - JBF_FilterKernelHalfSize);
								int kernel_right = std::min(frame_width - 1, column + JBF_FilterKernelHalfSize);
								int kernel_bottom = std::max(0, row - JBF_FilterKernelHalfSize);
								int kernel_top = std::min(frame_height - 1, row + JBF_FilterKernelHalfSize);

								glm::vec3 kernel_center_color = g_buffer.pixel_color(column, row);
								glm::vec3 kernel_center_world_position = g_buffer.pixel_world_position(column, row);
								glm::vec3 kernel_center_world_surface_normal = g_buffer.pixel_world_surface_normal(column, row);

								float unnormalized_weight = 0.0f;

								for (int kernel_column = kernel_left; kernel_column <= kernel_right; kernel_column++)
								{
									for (int kernel_row = kernel_bottom; kernel_row <= kernel_top; kernel_row++)

										// for each pixel in the filter kernel:

									{
										if (!(g_buffer.contributor(kernel_column, kernel_row)))
										{
											continue;
										}

										glm::vec3 kernel_pixel_color = g_buffer.pixel_color(kernel_column, kernel_row);
										glm::vec3 kernel_pixel_world_position = g_buffer.pixel_world_position(kernel_column, kernel_row);
										glm::vec3 kernel_pixel_world_surface_normal = g_buffer.pixel_world_surface_normal(kernel_column, kernel_row);

										if ((kernel_column == column) && (kernel_row == row))
										{
											unnormalized_weight += 1.0f;	// since in such case all the distances equal zero
											filtered_pixel_color += kernel_center_color;
											continue;
										}

										glm::vec3 dp = kernel_pixel_world_position - kernel_center_world_position;
										float world_position_distance = glm::dot(dp, dp) / (2.0f * sigma_position * sigma_position);

										glm::vec3 dc = kernel_pixel_color - kernel_center_color;
										float color_distance = glm::dot(dc, dc) / (2.0f * sigma_color * sigma_color);

										float surface_normal_distance = std::acos(std::min(std::max(0.0f, glm::dot(kernel_pixel_world_surface_normal, kernel_center_world_surface_normal)), 1.0f));
										surface_normal_distance *= surface_normal_distance;
										surface_normal_distance /= 2.0f * sigma_normal * sigma_normal;

										float coplanarity_distance = glm::dot(kernel_center_world_surface_normal, glm::normalize(dp));
										coplanarity_distance *= coplanarity_distance;
										coplanarity_distance /= 2.0f * sigma_coplanarity * sigma_coplanarity;

										float weight = std::exp(-(world_position_distance + color_distance + surface_normal_distance + coplanarity_distance));
										unnormalized_weight += weight;
										filtered_pixel_color += weight * kernel_pixel_color;
									}
								}

								if (!immediate_clamp)
								{
									// if we don't want to clamp the result:
									filtered_frame_buffer(column, row) = filtered_pixel_color / unnormalized_weight;
								}
								else
								{
									// otherwise:
									filtered_pixel_color = filtered_pixel_color / unnormalized_weight;
									filtered_frame_buffer(column, row) = glm::clamp(filtered_pixel_color, glm::vec3(0.0f), glm::vec3(1.0f));
								}
								
							}
							
						}
					);
				}
			);
			g_buffer.pixel_color = filtered_frame_buffer;
		}

		//void BackProjection(const G_Buffer& g_buffer)
		//{
		//	// {from canonical cube to screen} [P_previous] [V_previous] (World_position,1)
		//}

		void TemporalFiltering(G_Buffer& g_buffer, FrameBuffer<glm::vec3>& filtered_frame_buffer)
		{
			if (!using_temporal_filtering)
			{
				filtered_frame_buffer = g_buffer.pixel_color;
				accessible_previous_frame = false;
				return;
			}

			if (accessible_previous_frame)
			{
				/*
				A few things to note:

				We assume an accessible previous frame to have the same frame_width and frame_height as the current frame.

				The current version of temporal denoising is designed specifically for rendering the cornell box. Hence,
				we assume that camera is the ONLY thing that can move between any two frames. Thus, world position (of the
				objects in the scene) in current frame equals the corresponding world position in the previous frame.

				*/

				std::for_each(std::execution::par, rows.begin(), rows.end(),
					[&](int y)
					{
						std::for_each(std::execution::par, columns.begin(), columns.end(),
						[&](int x)
							{
								glm::vec3 color_from_previous_frame_pixel{0.0f, 0.0f, 0.0f};
								float temporal_blending_factor = 1.0f;	// 1.0 means all contributions are from current frame
								int id = g_buffer.primitive_id(x, y);

								if (id != -1)	// we don't need to denoise any pixels that do not hit anything
								{
									// For previous frame:
									glm::vec4 world_position {g_buffer.pixel_world_position(x, y), 1.0f};
									glm::vec4 canonical_position_4D = previous_frame_g_buffer.projection_matrix * (previous_frame_g_buffer.view_matrix * world_position);
									glm::vec2 canonical_position_2D { glm::vec3{ canonical_position_4D } / canonical_position_4D.w };
									glm::vec2 screen_position = (canonical_position_2D + 1.0f) / 2.0f;
									glm::vec2 pixel_position {screen_position.x* frame_width, screen_position.y* frame_height};

									if (pixel_position.x > 0.0f && pixel_position.x < frame_width && pixel_position.y > 0.0f && pixel_position.y < frame_height)
									{
										if (id == previous_frame_g_buffer.primitive_id((int)pixel_position.x, (int)pixel_position.y))
										{
											color_from_previous_frame_pixel = previous_frame_g_buffer.pixel_color(pixel_position.x, pixel_position.y);
											temporal_blending_factor = current_frame_weighting;

											int kernel_left = std::max(0, x - Temporal_FilterKernelHalfSize);
											int kernel_right = std::min(frame_width - 1, x + Temporal_FilterKernelHalfSize);
											int kernel_bottom = std::max(0, y - Temporal_FilterKernelHalfSize);
											int kernel_top = std::min(frame_height - 1, y + Temporal_FilterKernelHalfSize);

											glm::vec3 mean{0.0f, 0.0f, 0.0f};
											glm::vec3 variance{0.0f, 0.0f, 0.0f};
											int n = 0;
											for (int i = kernel_left; i <= kernel_right; i++)
											{
												for (int j = kernel_bottom; j <= kernel_top; j++)
												{
													n++;
													
													mean += g_buffer.pixel_color(i, j);

													glm::vec3 diff = g_buffer.pixel_color(x, y) - g_buffer.pixel_color(i, j);
													variance += diff * diff;
												}
											}
											mean = mean / (float)n;
											variance.x = std::sqrt(std::max(((variance.x) / (float)n), 0.0f));
											variance.y = std::sqrt(std::max(((variance.y) / (float)n), 0.0f));
											variance.z = std::sqrt(std::max(((variance.z) / (float)n), 0.0f));

											color_from_previous_frame_pixel = glm::clamp(color_from_previous_frame_pixel, mean - (tolerance * variance), mean + (tolerance * variance));
										}
									}
								}
								
								filtered_frame_buffer(x, y) = ((1.0f - temporal_blending_factor) * color_from_previous_frame_pixel) + (temporal_blending_factor * g_buffer.pixel_color(x, y));
								
							}
						);
					}
				);
				g_buffer.pixel_color = filtered_frame_buffer;
			}
			else
			{
				filtered_frame_buffer = g_buffer.pixel_color;
			}

			previous_frame_g_buffer = g_buffer;
			accessible_previous_frame = true;
		}

	public:

		bool using_JBF_filtering = true;
		int JBF_FilterKernelHalfSize = 7;	// try 7, 16, 32
		
		bool using_temporal_filtering = true;
		int Temporal_FilterKernelHalfSize = 3;
		float tolerance = 1.0f;		// how many variance is the previous frame pixel color allowed to deviate from the mean color in the corresponding temporal kernel of the current frame 
		float current_frame_weighting = 0.2f;	// for temporal filtering

		bool accessible_previous_frame = false;

	private:

		int frame_height = 0;
		int frame_width = 0;
		std::vector<int> rows;
		std::vector<int> columns;

		G_Buffer previous_frame_g_buffer;

		// Heuristic constants:

		float sigma_position = 32.0f;
		float sigma_color = 0.6f;
		float sigma_normal = 0.1f;
		float sigma_coplanarity = 0.1f;

	};
}

#endif // !DENOISER_H
