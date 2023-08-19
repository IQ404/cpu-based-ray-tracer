/*****************************************************************//**
 * \file   Material.h
 * \brief  The abstract class for materials
 * 
 * \author Xiaoyang Liu
 * \date   May 2023
 *********************************************************************/

#ifndef MATERIAL_H
#define MATERIAL_H

#include "RayTracingToolbox.h"

class HitRecord;

class Material
{
public:
	virtual bool scatter(const Ray& incident_ray, const HitRecord& record, Vector3D& attenuation, Ray& scattered_ray) const = 0;
};

#endif // !MATERIAL_H