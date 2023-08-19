/*****************************************************************//**
 * \file   CompositeHittable.cpp
 * \brief  The definitions of the class representing hittable that is made by hittables
 * 
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#include "CompositeHittable.h"

bool CompositeHittable::is_hit_by(const Ray& ray, double t_min, double t_max, HitRecord& record) const
{
	HitRecord component_record;
	bool is_hit = false;
	double visibility = t_max;

	for (const auto& component : components)
	{
		if (component->is_hit_by(ray, t_min, visibility, component_record))
		{
			is_hit = true;
			visibility = component_record.t;
			record = component_record;
		}
	}

	return is_hit;
}
