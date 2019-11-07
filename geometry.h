#pragma once
#include "linear_algebra.h"

// padding with dummy variables are required for memory alignment
// float3 is considered as float4 by OpenCL
// alignment can also be enforced by using __attribute__ ((aligned (16)));
// see https://www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/attributes-variables.html

struct Sphere
{
	float radius;
	int dummy1;
	float dummy2;
	float dummy3;
	Vector3Df position;
	Vector3Df emission;

	// added
	Vector3Df ambient;
	Vector3Df diffuse;
	Vector3Df specular; 
	float shininess;
	int dum1;
	float dum2;
	float dum3;
};
struct Light{
	Vector3Df position;
	Vector3Df color;
};
