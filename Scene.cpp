/*  This file defines the scene to render   */

#include "geometry.h"


const int sphere_num = 5;

// q learning segmentation
const float y_min = -0.2f;
const float y_max = 3.0f;
const float x_min = -6.0f;
const float x_max = 6.0f;
const float z_min = -6.0f;
const float z_max = 6.0f;
const float section = 0.05; // the size of the segment cube
const int y_size = 64;// (y_max - y_min) / section;
const int x_size = 240;// (x_max - x_min) / section;
const int z_size = 240;// (z_max - z_min) / section;

const int Qtable_size = x_size * y_size * z_size;




void initScene(Sphere* cpu_spheres) {
	// floor
	cpu_spheres[0].radius = 200.0f;
	cpu_spheres[0].position = Vector3Df(0.0f, -200.0f, 0.0f);
	cpu_spheres[0].specular = Vector3Df(0.9f, 0.4f, 0.3f);
	cpu_spheres[0].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[0].shininess = 0;

	// spheres
	cpu_spheres[1].radius = 0.16f;
	cpu_spheres[1].position = Vector3Df(-1.0f, 0.16f, 1.0f);
	cpu_spheres[1].specular = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[1].emission = Vector3Df(3.0f, 2.0f, 2.0f);
	cpu_spheres[1].shininess = 0;
	/*
	cpu_spheres[2].radius = 0.16f;
	cpu_spheres[2].position = Vector3Df(0.25f, 0.16f, 0.1f);
	cpu_spheres[2].specular = Vector3Df(0.7f, 0.9f, 0.7f);
	cpu_spheres[2].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[2].shininess = 1;*/

	cpu_spheres[2].radius = .4f;
	cpu_spheres[2].position = Vector3Df(-.4f, 0.2f, .4f);
	cpu_spheres[2].specular = Vector3Df(0.9f, 0.9f, 0.9f);
	cpu_spheres[2].emission = Vector3Df(.0f, .0f, .0f);
	cpu_spheres[2].shininess = 0;

	cpu_spheres[4].radius = 0.3f;
	cpu_spheres[4].position = Vector3Df(1.2f, 0.28f, -0.2f);
	cpu_spheres[4].specular = Vector3Df(0.7f, 0.9f, 0.7f);
	cpu_spheres[4].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[4].shininess = 100;

	// lightsource
	cpu_spheres[3].radius = 1.0f;
	cpu_spheres[3].position = Vector3Df(.7f, 1.6f, -.7f);
	cpu_spheres[3].specular = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[3].emission = Vector3Df(6.9f, 6.9f, 6.f);
	cpu_spheres[3].shininess = 0;

	/*
	cpu_spheres[4].radius = 1.0f;
	cpu_spheres[4].position = Vector3Df(-0.0f, 1.8f, -0.0f);
	cpu_spheres[4].specular = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[4].emission = Vector3Df(4.9f, 4.02f, 2.16f);
	cpu_spheres[4].shininess = 0;
	*/


	// 4 walls
	/*
	cpu_spheres[6].radius = 200.0f;
	cpu_spheres[6].position = Vector3Df(205.0f, 0.0f, 0.0f);
	cpu_spheres[6].specular = Vector3Df(0.9f, 0.4f, 0.3f);
	cpu_spheres[6].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[6].shininess = 0;
	cpu_spheres[7].radius = 200.0f;
	cpu_spheres[7].position = Vector3Df(-205.0f, 0.0f, 0.0f);
	cpu_spheres[7].specular = Vector3Df(0.9f, 0.4f, 0.3f);
	cpu_spheres[7].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[7].shininess = 0;
	cpu_spheres[8].radius = 200.0f;
	cpu_spheres[8].position = Vector3Df(0.0f, 0.0f, 205.0f);
	cpu_spheres[8].specular = Vector3Df(0.9f, 0.4f, 0.3f);
	cpu_spheres[8].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[8].shininess = 0;
	cpu_spheres[9].radius = 200.0f;
	cpu_spheres[9].position = Vector3Df(0.0f, 0.0f, -205.0f);
	cpu_spheres[9].specular = Vector3Df(0.9f, 0.4f, 0.3f);
	cpu_spheres[9].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[9].shininess = 0;
	*/
}