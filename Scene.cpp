/*  This file defines the scene to render   */

#include "geometry.h"

const int light_num = 1;
const int sphere_matrix_side = 3;
const float sphere_matrix_interrum = 0.7;

const int sphere_num  = sphere_matrix_side*sphere_matrix_side + 1;

void initScene(Sphere* cpu_spheres, Light* cpu_lights) {

    // matrix of spheres on the floor
    for (int i =0; i< sphere_matrix_side; i++){
        for(int j =0; j < sphere_matrix_side; j++){
            int sphere_index = i*sphere_matrix_side + j;
            // set paramters
            cpu_spheres[sphere_index].radius = 0.16f;
            cpu_spheres[sphere_index].position = 
                Vector3Df(j*sphere_matrix_interrum, -0.24f, -i*sphere_matrix_interrum);
            // differnent material
            if ((j+i)%2==1){
                cpu_spheres[sphere_index].emission = Vector3Df(0.0f, 0.0f, 0.0f);
                cpu_spheres[sphere_index].ambient = Vector3Df(0.03125, 0.13125, 0.13125);
                cpu_spheres[sphere_index].diffuse = Vector3Df(0.2775, 0.2775, 0.2775);
                cpu_spheres[sphere_index].specular = Vector3Df(0.773911, 0.773911, 0.773911);
                cpu_spheres[sphere_index].shininess = 90.0f;
            }
            else{
                cpu_spheres[sphere_index].emission = Vector3Df(0.0f, 0.0f, 0.0f);
                cpu_spheres[sphere_index].ambient = Vector3Df(0.0f, 0.05f, 0.0f);
                cpu_spheres[sphere_index].diffuse = Vector3Df(64.0f/256.0f, 163.0f/256.0f, 90.0f/256.0f);
                cpu_spheres[sphere_index].specular = Vector3Df(0.2f, 0.2f, 0.2f);
                cpu_spheres[sphere_index].shininess = 20.0f;
            }
        }
    }
    // change one to be differnent size(for fun)
    cpu_spheres[sphere_num/2-1].radius = 0.4f;
    cpu_spheres[sphere_num/2-1].emission = Vector3Df(0.0f, 0.0f, 0.0f);
    cpu_spheres[sphere_num/2-1].ambient = Vector3Df(0.03125, 0.03125, 0.03125);
    cpu_spheres[sphere_num/2-1].diffuse =Vector3Df(0.1775, 0.1775, 0.1775);
    cpu_spheres[sphere_num/2-1].specular = Vector3Df(0.773911, 0.773911, 0.773911);
    cpu_spheres[sphere_num/2-1].shininess = 100.2f;
    cpu_spheres[sphere_num/2-1].position = 
        Vector3Df(sphere_matrix_side/2*sphere_matrix_interrum, 0.5f, -sphere_matrix_side/2*sphere_matrix_interrum);


    // floor (the last sphere)
	cpu_spheres[sphere_num-1].radius = 200.0f;
	cpu_spheres[sphere_num-1].position = Vector3Df(0.0f, -200.4f, 0.0f);
	cpu_spheres[sphere_num-1].emission = Vector3Df(0.0f, 0.0f, 0.0f);
	cpu_spheres[sphere_num-1].ambient = Vector3Df(0.25f, 0.20725f, 0.20725f);
	cpu_spheres[sphere_num-1].diffuse = Vector3Df(1.0f, 0.829, 0.829);
	cpu_spheres[sphere_num-1].specular = Vector3Df(0.1, 0.1, 0.1);
	cpu_spheres[sphere_num-1].shininess = 11.264;

	// top directional light
	cpu_lights[0].position = Vector3Df(0.0f, 1.0f, -1.0f);
	cpu_lights[0].color = Vector3Df(0.4f, 0.4f, 0.2f);
}