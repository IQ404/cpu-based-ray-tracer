/*****************************************************************//**
 * \file   Hittable.h
 * \brief  The abstract class for anything that can be hitted by the ray
 * 
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#ifndef HITTABLE_H
#define HITTABLE_H

#include "Ray.h"
#include "HitRecord.h"

class Hittable
{
public:
	virtual bool is_hit_by(const Ray& ray, double t_min, double t_max, HitRecord& record) const = 0;
};

#endif // !HITTABLE_H