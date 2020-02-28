/*  This file defines the scene to render   */

#include "geometry.h"


const int sphere_num  = 6;

void initScene(Sphere* cpu_spheres) {
	// floor
	cpu_spheres[0].radius = 200.0f;
	cpu_spheres[0].position = Vector3Df(0.0f, -200.4f, 0.0f);
	cpu_spheres[0].specular = Vector3Df(0.9f, 0.4f, 0.3f);
	cpu_spheres[0].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[0].shininess = 0;

	// left sphere
	cpu_spheres[1].radius = 0.16f;
	cpu_spheres[1].position = Vector3Df(-0.25f, -0.24f, -0.1f);
	cpu_spheres[1].specular = Vector3Df(0.6f, 0.7f, 0.9f);
	cpu_spheres[1].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[1].shininess = 100;

	// right sphere
	cpu_spheres[2].radius = 0.16f;
	cpu_spheres[2].position = Vector3Df(0.25f, -0.24f, 0.1f);
	cpu_spheres[2].specular = Vector3Df(0.7f, 0.9f, 0.7f);
	cpu_spheres[2].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[2].shininess = 1;

	// lightsource
	cpu_spheres[3].radius = .4f;
	cpu_spheres[3].position = Vector3Df(-.5f, -0.2f, .5f);
	cpu_spheres[3].specular = Vector3Df(0.9f, 0.9f, 0.9f);
	cpu_spheres[3].emission = Vector3Df(.0f, .0f, .0f);
	cpu_spheres[3].shininess = 0;

	cpu_spheres[4].radius = 0.3f;
	cpu_spheres[4].position = Vector3Df(.5f, -.2f, -.5f);
	cpu_spheres[4].specular = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[4].emission = Vector3Df(4.f, 4.f, 4.f);
	cpu_spheres[4].shininess = 0;

	cpu_spheres[5].radius = 1.0f;
	cpu_spheres[5].position = Vector3Df(-0.0f, 1.6f, -0.0f);
	cpu_spheres[5].specular = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[5].emission = Vector3Df(4.9f, 4.02f, 2.16f);
	cpu_spheres[5].shininess = 0;

}