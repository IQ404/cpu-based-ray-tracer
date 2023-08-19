/*****************************************************************//**
 * \file   CompositeHittable.h
 * \brief  The declaration of the class representing hittable that is made by hittables
 * 
 * \author Xiaoyang Liu
 * \date   April 2023
 *********************************************************************/

#ifndef COMPOSITEHITTABLE_H
#define COMPOSITEHITTABLE_H

#include <memory>	// to use smart pointers
#include <vector>
#include "Hittable.h"

/*
C++ side notes on shared pointers:
	
	std::shared_ptr<T> is an pointer encapsulated with reference-counting semantics such that it is automatically deleted when the counting is 0.
	
	shared_ptr<T> sptr = make_shared<T>(T_constructor_params ...);	// make_shared allocates the object on the heap

	Moreover, there is no way to manage a stack allocated object with a shared pointer!
*/

class CompositeHittable : public Hittable
{
	std::vector<std::shared_ptr<Hittable>> components;
	
public:

	// Constructors:

	CompositeHittable()
	{

	}

	CompositeHittable(std::shared_ptr<Hittable> hittable_object)
	{
		components.push_back(hittable_object);
	}

	// Methods:

	void clear()
	{
		components.clear();
	}

	void add(std::shared_ptr<Hittable> hittable_object)
	{
		components.push_back(hittable_object);
	}

	virtual bool is_hit_by(const Ray& ray, double t_min, double t_max, HitRecord& record) const override;
};

#endif // !COMPOSITEHITTABLE_H
