/*****************************************************************//**
 * \file   TriangleMesh.h
 * \brief  Representation of a mesh consisting of triangle-primitives in a Whitted-Style ray-traced scene
 * 
 * \author Xiaoyang Liu
 * \date   June 2023
 *********************************************************************/

#ifndef TRIANGLEMESH_H
#define TRIANGLEMESH_H

#include <cassert>

#include "OBJ_Loader.h"
#include "BVH.h"

namespace Whitted
{
	inline bool RayTriangleIntersection(
		const glm::vec3& vertice_1, 
		const glm::vec3& vertice_2, 
		const glm::vec3& vertice_3,
		const glm::vec3& ray_origin,
		const glm::vec3& ray_direction,
		double& t_intersection
		)
	// implemented using Moller-Trumbore algorithm (TODO: get a pen and a paper to derive the math behind this algorithm!!!)
	{
		glm::vec3 E_1 = vertice_2 - vertice_1;
		glm::vec3 E_2 = vertice_3 - vertice_1;
		glm::vec3 S = ray_origin - vertice_1;
		glm::vec3 S_1 = glm::cross(ray_direction, E_2);
		glm::vec3 S_2 = glm::cross(S, E_1);

		double inverse_denominator = 1.0 / glm::dot(S_1, E_1);

		t_intersection = glm::dot(S_2, E_2) * inverse_denominator;
		double barycentric_coordinate_2 = glm::dot(S_1, S) * inverse_denominator;
		double barycentric_coordinate_3 = glm::dot(S_2, ray_direction) * inverse_denominator;
		// Note: barycentric_coordinate_1 = 1 - barycentric_coordinate_2 - barycentric_coordinate_3
		return ((t_intersection > 0.0) && 
			(barycentric_coordinate_2 > 0.0) && 
			(barycentric_coordinate_3 > 0.0) && 
			((1.0 - barycentric_coordinate_2 - barycentric_coordinate_3) > 0.0));	// TODO: add epsilon to account for floating point precision error?
	}

	class TrianglePrimitive : public Entity
	/*
	Assumptions:
	The vertices are declared in anti-clockwise order.
	*/
	{
	public:
		TrianglePrimitive(int& id_count, glm::vec3 _vertice_a, glm::vec3 _vertice_b, glm::vec3 _vertice_c, WhittedMaterial* _material = nullptr)
			: vertice_a(_vertice_a), vertice_b(_vertice_b), vertice_c(_vertice_c), material(_material)
		{
			glm::vec3 cross_product = glm::cross(_vertice_b - _vertice_a, _vertice_c - _vertice_a);
			area = 0.5f * glm::length(cross_product);
			m_surface_normal = Whitted::normalize(cross_product);

			id = id_count;
			id_count++;
		}

		// Methods:

		virtual float GetArea() override
		{
			return area;
		}

		virtual void Sampling(IntersectionRecord& sample, float& PDF) override
		// TODO: do we really need PDF here (given we are not using any non-uniform PDF)?
		{
			/*
			We need an algorithm to do random sampling on an triangle.
			We can do that by computing the three components of a barycentric coordinates.
			To determine three unknowns (say x,y,z), we need to fix at least two of them. Let's say we will fix x and y.
			*/
			float x = 1 - std::sqrt(get_random_float_0_1());
			float y = get_random_float_0_1();
			/*
			The sqrt term above (is likely to) give us a relatively bigger number (closer to 1) and thus a smaller x (closer to 0).
			We do that because we are partitioning 1 into three pieces (rather than two), and thus we don't want a bias on x.
			Now, we can cut the remaining area (i.e. 1-x) into two using y as the percentage.
			*/
			sample.location = (x)*vertice_a + ((1.0f - x) * (y)) * vertice_b + ((1.0f - x) * (1.0f - y)) * vertice_c;
			
			sample.surface_normal = m_surface_normal;
			
			PDF = 1.0f / area;	// see my MSc dissertation for derivation.
		}

		virtual bool IsEmissive() override
		{
			return material->IsEmitting();
		}

		virtual AccelerationStructure::AABB_3D Get3DAABB() override
		{
			return (AccelerationStructure::AABB_3D{vertice_a, vertice_b}).Union_with_point(vertice_c);
		}

		virtual glm::vec3 GetDiffuseColor(const glm::vec2&) const override
		{
			return glm::vec3{0.5f, 0.5f, 0.5f};		// TODO: whenever possible, use material information instead?
		}

		virtual void GetHitInfo(
			const glm::vec3& intersection,
			const glm::vec3& light_direction,
			const uint32_t& triangle_index,
			const glm::vec2& barycentric_coordinates,
			glm::vec3& surface_normal,
			glm::vec2& texture_coordinates
		) const override
		{
			surface_normal = m_surface_normal;
		}

		virtual IntersectionRecord GetIntersectionRecord(AccelerationStructure::Ray ray) override
		{
			IntersectionRecord record;
			if (RayTriangleIntersection(vertice_a, vertice_b, vertice_c, ray.m_origin, ray.m_direction, record.t))
			{
				record.has_intersection = true;
				record.hitted_entity_material = material;
				record.hitted_entity = this;
				record.surface_normal = m_surface_normal;
				record.location = ray(record.t);	// Note that this is calling the operator()
				record.primitive_id = this->id;
			}
			else
			{
				record.t = std::numeric_limits<double>::max();
			}
			return record;
		}

	public:		// Data members:
		float area;
		glm::vec3 vertice_a;
		glm::vec3 vertice_b;
		glm::vec3 vertice_c;
		glm::vec3 m_surface_normal;
		WhittedMaterial* material;
	};

	class TriangleMesh : public Entity
	{
	public:
		TriangleMesh(int& id_count, const std::string& file_path, WhittedMaterial* m)
		{
			constexpr float mesh_scale = 0.01f;

			unified_material = m;
			total_area = 0.0f;

			objl::Loader Robert_Smith_Loader;
			Robert_Smith_Loader.LoadFile(file_path);
			// assume only one mesh is loaded...
			assert(Robert_Smith_Loader.LoadedMeshes.size() == 1);

			objl::Mesh loaded_mesh = Robert_Smith_Loader.LoadedMeshes[0];
			
			glm::vec3 mesh_range_min = glm::vec3{ std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity() };
			glm::vec3 mesh_range_max = glm::vec3{ -std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity() };

			for (int i = 0; i < loaded_mesh.Vertices.size(); i += 3)	// one loop for each triangle
			{
				std::array<glm::vec3, 3> current_triangle;
				for (int j = 0; j < 3; j++)
				{
					glm::vec3 scaled_vertice = mesh_scale * glm::vec3{loaded_mesh.Vertices[i + j].Position.X, loaded_mesh.Vertices[i + j].Position.Y, loaded_mesh.Vertices[i + j].Position.Z };
					current_triangle[j] = scaled_vertice;

					mesh_range_min = glm::vec3{ std::min(mesh_range_min.x, scaled_vertice.x), std::min(mesh_range_min.y, scaled_vertice.y), std::min(mesh_range_min.z, scaled_vertice.z) };
					mesh_range_max = glm::vec3{ std::max(mesh_range_max.x, scaled_vertice.x), std::max(mesh_range_max.y, scaled_vertice.y), std::max(mesh_range_max.z, scaled_vertice.z) };
				}
				triangle_primitives.emplace_back(id_count, current_triangle[0], current_triangle[1], current_triangle[2], unified_material);	// implicitly calls TrianglePrimitive's constructor
			}
			bounding_AABB = AccelerationStructure::AABB_3D{ mesh_range_min,mesh_range_max };
			std::vector<Entity*> entity_pointers;
			for (TrianglePrimitive& triangle : triangle_primitives)
			{
				total_area += triangle.area;
				entity_pointers.push_back(&triangle);
			}
			bvh = new AccelerationStructure::BVH{ entity_pointers };
		}

		virtual float GetArea() override
		{
			return total_area;
		}

		virtual void Sampling(IntersectionRecord& sample, float& PDF) override
		{
			sample.emission = unified_material->GetEmission();
			bvh->Sampling_from_root(sample, PDF);
		}

		virtual bool IsEmissive() override
		{
			return unified_material->IsEmitting();
		}

		virtual AccelerationStructure::AABB_3D Get3DAABB() override
		{
			return bounding_AABB;
		}

		virtual IntersectionRecord GetIntersectionRecord(AccelerationStructure::Ray ray) override
		{
			IntersectionRecord record;
			if (bvh)
			{
				record = bvh->traverse_BVH_from_root(ray);
			}
			return record;
		}

		virtual glm::vec3 GetDiffuseColor(const glm::vec2& texture_coordinates) const override
		{
			// A procedural texture algorithm generating chessboard-like pattern for the floor
			float frequency = 5;
			float pattern = (std::fmodf(texture_coordinates.x * frequency, 1) > 0.5) ^ (std::fmodf(texture_coordinates.y * frequency, 1) > 0.5);
			// See https://learn.microsoft.com/en-us/cpp/cpp/bitwise-exclusive-or-operator-hat?view=msvc-170
			return Whitted::lerp(glm::vec3(0.815, 0.235, 0.031), glm::vec3(0.937, 0.937, 0.231), pattern);
		}

		virtual void GetHitInfo(
			const glm::vec3&,
			const glm::vec3&,
			const uint32_t& triangle_index,
			const glm::vec2& barycentric_coordinates,
			glm::vec3& surface_normal,
			glm::vec2& texture_coordinates
		) const override
		{
			const glm::vec3& vertice_1 = m_vertices[m_vertices_indices[triangle_index * 3]];
			const glm::vec3& vertice_2 = m_vertices[m_vertices_indices[triangle_index * 3 + 1]];
			const glm::vec3& vertice_3 = m_vertices[m_vertices_indices[triangle_index * 3 + 2]];
			// Note that triangle_index starts from 0
			surface_normal = Whitted::normalize(
				glm::cross(
					Whitted::normalize(vertice_2 - vertice_1), Whitted::normalize(vertice_3 - vertice_2)
				)
			);
			// Note: we computed this normal with the orientation that is according to the right-hand rule 
			// because we want to find the "outward" direction.

			const glm::vec2& texture_coordinates_vertice_1 = m_texture_coordinates[m_vertices_indices[triangle_index * 3]];
			const glm::vec2& texture_coordinates_vertice_2 = m_texture_coordinates[m_vertices_indices[triangle_index * 3 + 1]];
			const glm::vec2& texture_coordinates_vertice_3 = m_texture_coordinates[m_vertices_indices[triangle_index * 3 + 2]];

			texture_coordinates =
				(1 - barycentric_coordinates.x - barycentric_coordinates.y) * texture_coordinates_vertice_1 +
				barycentric_coordinates.x * texture_coordinates_vertice_2 +
				barycentric_coordinates.y * texture_coordinates_vertice_3;
		}

	private:
		float total_area;
		WhittedMaterial* unified_material = nullptr;	// Now we want all the triangles in one mesh to have the same material
		std::vector<TrianglePrimitive> triangle_primitives;
		std::unique_ptr<glm::vec3[]> m_vertices;
		std::unique_ptr<glm::vec2[]> m_texture_coordinates;
		std::unique_ptr<uint32_t[]> m_vertices_indices;
		AccelerationStructure::AABB_3D bounding_AABB;
		AccelerationStructure::BVH* bvh;
	};
}

#endif // !TRIANGLEMESH_H
