/* Modified from:
/* simple sphere path tracer by Sam Lapere, 2016*/
/* http://raytracey.blogspot.com */
/* interactive camera and depth-of-field code based on GPU path tracer by Karl Li and Peter Kutz */

__constant float EPSILON = 0.0003f; /* req2uired to compensate for limited float precision */
__constant float PI = 3.14159265359f;
__constant bool ANTI_ALIAS = true;
__constant int RECURSE_DEPTH = 4;
__constant float SPECULAR_THRESHOLD = 0.1; /* stop recursive when specular too low */
__constant float3 background_color = (float3)(0,0,0);/*(float3)(30/256.0f, 65.0f/256.0f, 60.0f/256.0f);*/

typedef struct Ray{
	float3 origin;
	float3 dir;
} Ray;

typedef struct Light{
	float3 pos;
	float3 color;
} Light;

typedef struct Sphere{
	float radius;
	float3 pos;
	float3 emission;
	/* added */
	float3 ambient;
	float3 diffuse;
	float3 specular; 
	float shinniness;

} Sphere;

typedef struct Camera{
	float3 position;		
	float3 view;			
	float3 up;			
	float2 resolution;	
	float2 fov;		
	float apertureRadius;
	float focalDistance;
} Camera;

Ray createCamRay(const int x_coord, const int y_coord, const int width, const int height, __constant Camera* cam){

	/* create a local coordinate frame for the camera */
	float3 rendercamview = cam->view; rendercamview = normalize(rendercamview);
	float3 rendercamup = cam->up; rendercamup = normalize(rendercamup);
	float3 horizontalAxis = cross(rendercamview, rendercamup); horizontalAxis = normalize(horizontalAxis);
	float3 verticalAxis = cross(horizontalAxis, rendercamview); verticalAxis = normalize(verticalAxis);

	float3 middle = cam->position + rendercamview;
	float3 horizontal = horizontalAxis * tan(cam->fov.x * 0.5f * (PI / 180)); 
	float3 vertical   =  verticalAxis * tan(cam->fov.y * -0.5f * (PI / 180)); 

	unsigned int x = x_coord;
	unsigned int y = y_coord;

	int pixelx = x_coord; 
	int pixely = height - y_coord - 1;

	float sx = (float)pixelx / (width - 1.0f);
	float sy = (float)pixely / (height - 1.0f);
	
	float3 pointOnPlaneOneUnitAwayFromEye = middle + (horizontal * ((2 * sx) - 1)) + (vertical * ((2 * sy) - 1));
	float3 pointOnImagePlane = cam->position + ((pointOnPlaneOneUnitAwayFromEye - cam->position) * cam->focalDistance); /*cam->focalDistance*/ 

	float3 aperturePoint;
	aperturePoint = cam->position;

	float3 apertureToImagePlane = pointOnImagePlane - aperturePoint; apertureToImagePlane = normalize(apertureToImagePlane); 

	/* create camera ray*/
	Ray ray;
	ray.origin = aperturePoint;
	ray.dir = apertureToImagePlane; 

	return ray;
}

/* (__global Sphere* sphere, const Ray* ray) */
float intersect_sphere(const Sphere* sphere, const Ray* ray) /* version using local copy of sphere */
{
	float3 rayToCenter = sphere->pos - ray->origin;
	float b = dot(rayToCenter, ray->dir);
	float c = dot(rayToCenter, rayToCenter) - sphere->radius*sphere->radius;
	float disc = b * b - c;

	if (disc < 0.0f) return 0.0f;
	else disc = sqrt(disc);

	if ((b - disc) > EPSILON) return b - disc;
	if ((b + disc) > EPSILON) return b + disc;

	return 0.0f;
}

bool intersect_scene(__constant Sphere* spheres, const Ray* ray, float* t, int* sphere_id, const int sphere_count)
{
	/* initialise t to a very large number,
	so t will be guaranteed to be smaller
	when a hit with the scene occurs */

	float inf = 1e20f;
	*t = inf;

	/* check if the ray intersects each sphere in the scene */
	for (int i = 0; i < sphere_count; i++)  {

		Sphere sphere = spheres[i]; /* create local copy of sphere */

		/* float hitdistance = intersect_sphere(&spheres[i], ray); */
		float hitdistance = intersect_sphere(&sphere, ray);
		/* keep track of the closest intersection and hitobject found so far */
		if (hitdistance != 0.0f && hitdistance < *t) {
			*t = hitdistance;
			*sphere_id = i;
		}
	}
	return *t < inf; /* true when ray interesects the scene */
}

/* compute the coloring from light */
float3 computeLight(const float3 direction, const float3 lightcolor, const float3 normal,
	const float3 halfvec, const float3 mydiffuse, const float3 myspecular, const float myshininess) 
	{
	float nDotL = dot(normal, direction);
	float3 lambert = mydiffuse * lightcolor * max(nDotL, 0.0f);

	float nDotH = dot(normal, halfvec);
	float3 phong = myspecular * lightcolor * pow(max(nDotH, 0.0f), myshininess);

	float3 retval = lambert + phong;
	return retval;
}

/* the path tracing function */
/* return the color thru the tracing process */
float3 trace(__constant Sphere* spheres, __constant Light* lights, const Ray* camray, 
	const int sphere_count, const int light_count){

	Ray ray = *camray;

	float3 accum_color = (float3)(0.0f, 0.0f, 0.0f);

	float t;   /* distance to intersection */
	int hitsphere_id = 0; /* index of intersected sphere */	
	float3 accum_specular = (float3)(1.0f, 1.0f, 1.0f); /* specular of prev objs */

	/* start the tracing process */
	for (int i = 0; i < RECURSE_DEPTH; i++){

		/* if ray misses scene, return background colour ON THE FIRST RAY*/
		bool intersect = intersect_scene(spheres, &ray, &t, &hitsphere_id, sphere_count);
		if ( !intersect && i==0) 
			return accum_color += background_color;
		/* if ray miss ON REFLECTION, break the loop */
		else if ( !intersect && i > 0){
			accum_color += background_color;
			break;
		}

		/* else, we've got a hit! Fetch the closest hit sphere */
		Sphere hitsphere = spheres[hitsphere_id]; /* version with local copy of sphere */
		/* compute the hitpoint using the ray equation */
		float3 hitpoint = ray.origin + ray.dir * t;

		/* compute the surface normal and flip it if necessary to face the incoming ray */
		float3 normal = normalize(hitpoint - hitsphere.pos);
		float3 normal_facing = dot(normal, ray.dir) < 0.0f ? normal : normal * (-1.0f);

		/* color from object itself */
		float3 obj_col = hitsphere.ambient + hitsphere.emission;
		/* color from lights */ 
		for (int i =0; i < light_count; i++){
			float3 eye_dir = -1.0f * ray.dir;
			float3 light_posn = lights[i].pos;
			float3 light_color = lights[i].color;
			/* we assume lights are all directional */
			Ray light_ray;
			light_ray.dir = normalize(light_posn);
			light_ray.origin = hitpoint + EPSILON * light_ray.dir;
			/* check if light blocked */
			float travel_time = 0.0f;
			float hit_id = 0;
			if ( ! intersect_scene(spheres, &light_ray, &travel_time, 
				&hit_id, sphere_count)   )
				{
					/* add light color if non-blocked */
					float3 H = normalize(eye_dir + light_ray.dir);
					float3 col_from_light = computeLight(
						light_ray.dir, light_color, normal_facing,
						H, hitsphere.diffuse, hitsphere.specular,
						hitsphere.shinniness
					);
					obj_col += col_from_light;
			}
		}
		accum_color += obj_col * accum_specular;

		/* start to compute the reflection */
		/* update the ray */
		ray.origin = hitpoint + normal_facing * EPSILON;
		ray.dir = normalize(ray.dir - 2 * dot(ray.dir,normal_facing) * normal_facing);
		accum_specular *= hitsphere.specular;
		/* if reflection gets too slow, stop */
		if (length(accum_specular) < SPECULAR_THRESHOLD)
			break;
	}

	return accum_color;
}

union Colour{ float c; uchar4 components; };

__kernel void render_kernel(__constant Sphere* spheres, const int width, const int height, 
	const int sphere_count, __global float3* output,  const int framenumber,__constant const Camera* cam, 
	__global float3* accumbuffer, __constant Light* lights, const int light_count, __global float3* output2)
{
	unsigned int work_item_id = get_global_id(0);	/* the unique global id of the work item for the current pixel */
	unsigned int x_coord = work_item_id % width;			/* x-coordinate of the pixel */
	unsigned int y_coord = work_item_id / width;			/* y-coordinate of the pixel */

	float3 finalcolor = (float3)(0.0f, 0.0f, 0.0f);

	/* anti-alising */
	if (ANTI_ALIAS){
		for (unsigned int i = 0; i < 4; i++){
			Ray camray;
			if (i==0){
				camray = createCamRay(x_coord - 0.25, y_coord - 0.25, width, height, cam);
			}
			else if (i==1){
				camray = createCamRay(x_coord + 0.25, y_coord - 0.25, width, height, cam);
			}
			else if(i==2){
				camray = createCamRay(x_coord + 0.25, y_coord + 0.25, width, height, cam);
			}
			else{
				camray = createCamRay(x_coord - 0.25, y_coord + 0.25, width, height, cam);
			}
			finalcolor += trace(spheres, lights, &camray, sphere_count, light_count);
		}
		finalcolor = finalcolor / 4;
	}
	else{
		Ray camray = createCamRay(x_coord , y_coord , width, height, cam);
		finalcolor += trace(spheres, lights, &camray, sphere_count, light_count);
	}

	/* add pixel colour to accumulation buffer (accumulates all samples) */
	accumbuffer[work_item_id] += finalcolor;
	float3 tempcolor = accumbuffer[work_item_id] / framenumber; 

	tempcolor = (float3)(
		clamp(tempcolor.x, 0.0f, 1.0f),
		clamp(tempcolor.y, 0.0f, 1.0f), 
		clamp(tempcolor.z, 0.0f, 1.0f));

	union Colour fcolor;
	fcolor.components = (uchar4)(
		(unsigned char)(tempcolor.x * 255),
		(unsigned char)(tempcolor.y * 255),
		(unsigned char)(tempcolor.z * 255),
		1);

	/* store the pixelcolour in the output buffer */
	output[work_item_id] = (float3)(x_coord, y_coord, fcolor.c);
	output2[work_item_id] = tempcolor;
}
