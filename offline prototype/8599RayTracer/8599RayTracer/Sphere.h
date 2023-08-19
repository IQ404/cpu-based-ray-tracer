/*****************************************************************//**
 * \file   Sphere.h
 * \brief  Class declaration for ray-hittable sphere
 * 
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#ifndef SPHERE_H
#define SPHERE_H

#include "Hittable.h"

/*
C++ side note:
If we don't provide class access specifier in inheritance,
it would be public if the sub-class is declared as struct and be private if the sub-class is declared as class
*/

class Sphere : public Hittable
{
	Point3D center;
	double radius;
	std::shared_ptr<Material> material_pointer;

public:

	Sphere()	// currently, radius is uninitialized! what default value should we set?
	{

	}

	Sphere(Point3D centre, double r, std::shared_ptr<Material> material)
		: center{ centre }, radius{ r }, material_pointer{ material }
	{

	}

	virtual bool is_hit_by(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;
};

#endif // !SPHERE_H
