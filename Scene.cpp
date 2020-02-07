/*  This file defines the scene to render   */

#include "geometry.h"

const int sphere_matrix_side = 2;
const float sphere_matrix_interrum = 0.7;

const int sphere_num  = sphere_matrix_side*sphere_matrix_side + 1;

void initScene(Sphere* cpu_spheres) {

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
                cpu_spheres[sphere_index].specular = Vector3Df(0.9f, 0.8f, 0.7f);
                cpu_spheres[sphere_index].shininess = 120.0f;
            }
            else{
                cpu_spheres[sphere_index].emission = Vector3Df(0.5f, 0.5f, 0.5f);
                cpu_spheres[sphere_index].specular = Vector3Df(0.9f, 0.8f, 0.7f);
                cpu_spheres[sphere_index].shininess = 100.0f;
            }
        }
    }
    // change one to be differnent size(for fun)
    cpu_spheres[sphere_num/2-1].radius = 0.4f;
    cpu_spheres[sphere_num/2-1].emission = Vector3Df(.9,.8,.6);
    cpu_spheres[sphere_num/2-1].specular = Vector3Df(.0,.0,.0);
    cpu_spheres[sphere_num/2-1].shininess = 100.2f;
    cpu_spheres[sphere_num/2-1].position = 
        Vector3Df(sphere_matrix_side/2*sphere_matrix_interrum, 0.5f, -sphere_matrix_side/2*sphere_matrix_interrum);


    // floor (the last sphere)
	cpu_spheres[sphere_num-1].radius = 200.0f;
	cpu_spheres[sphere_num-1].position = Vector3Df(0.0f, -200.4f, 0.0f);
	cpu_spheres[sphere_num-1].emission = Vector3Df(.0,.0,.0);
	cpu_spheres[sphere_num-1].specular = Vector3Df(0.9, 0.8, 0.7);
	cpu_spheres[sphere_num-1].shininess = 20.264;

}