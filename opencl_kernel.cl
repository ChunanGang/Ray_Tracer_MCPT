

__constant float EPSILON = 0.0003f; /* req2uired to compensate for limited float precision */
__constant int RECURSE_DEPTH = 6;
__constant float PI = 3.14159265359f;
__constant float SPECULAR_THRESHOLD = 0.05; /* stop recursive when specular too low */
__constant float3 background_color = (float3)(0.0);

/* RL part */
/* standard directions that get mapped into */
__constant float3 standard_dirs[] = { (float3)(0.5774f, 0.5774f, 0.5774f),
	(float3)(-0.5774f, -0.5774f, -0.5774f) ,
	(float3)(0.5774f, -0.5774f, -0.5774f),
	(float3)(-0.5774f, 0.5774f, -0.5774f),
	(float3)(-0.5774f, -0.5774f, 0.5774f),
	(float3)(-0.5774f, 0.5774f, 0.5774f),
	(float3)(0.5774f, -0.5774f, 0.5774f),
	(float3)(0.5774f, 0.5774f, -0.5774f),

	(float3)(1.0f, 0.0f, 0.0f),
	(float3)(0.0f, 1.0f, 0.0f),
	(float3)(0.0f, 0.0f, 1.0f),
	(float3)(-1.0f, 0.0f, 0.0f),
	(float3)(0.0f, -1.0f, 0.0f),
	(float3)(0.0f, 0.0f, -1.0f),

	(float3)(0.0f, 0.7071f, 0.7071f),
	(float3)(0.0f, -0.7071f, -0.7071f),
	(float3)(0.7071f, 0.0f, 0.7071f),
	(float3)(-0.7071f, 0.0f, -0.7071f),
	(float3)(0.7071f, 0.7071f, 0.0f),
	(float3)(-0.7071f, -0.7071f, 0.0f),

	(float3)(0.0f, 0.7071f, -0.7071f),
	(float3)(0.0f, -0.7071f, 0.7071f),
	(float3)(-0.7071f, 0.0f, 0.7071f),
	(float3)(0.7071f, 0.0f, -0.7071f),
	(float3)(0.7071f, -0.7071f, 0.0f),
	(float3)(-0.7071f, 0.7071f, 0.0f),
};
/* q learning segmentation */
__constant float section = 0.05;
__constant float y_min = -0.2f;
__constant float y_max = 3.0f;
__constant float x_min = -6.0f;
__constant float x_max = 6.0f;
__constant float z_min = -6.0f;
__constant float z_max = 6.0f;
__constant int y_size = 32;
__constant int x_size =240;
__constant int z_size = 240;
/* q learning para */
__constant float LR = 0.001;
__constant float GAMMA = 0.8;

typedef struct Ray{
	float3 origin;
	float3 dir;
} Ray;

typedef struct Sphere{
	float radius;
	float3 pos;
	float3 emission;

	float3 specular; 
	float shininess;

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

typedef struct Q_Table_Node {
	float action[26] ; /* directions */
	float max ;
}Qnode;


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


static float get_random(unsigned int *seed0, unsigned int *seed1) {

	/* hash the seeds using bitwise AND operations and bitshifts */
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	unsigned int ires = ((*seed0) << 16) + (*seed1);

	/* use union struct to convert int to float */
	union {
		float f;
		unsigned int ui;
	} res;

	res.ui = (ires & 0x007fffff) | 0x40000000;  /* bitwise AND, bitwise OR */
	return (res.f - 2.0f) / 2.0f;
}

/* use the coordinte frame and random numbers to compute the next ray direction */
static float3 generate_rand_dir(int * seed0, int * seed1, float3 normal_facing, float3 reflect_dir, float shininess) {
	/* compute the next ray */
	float rand1 = 2.0f * PI * get_random(seed0, seed1);
	float rand2 = get_random(seed1, seed0);
	float rand2s = sqrt(rand2);
	/* create a local orthogonal coordinate frame centered at the hitpoint */
	float3 w = normal_facing;
	float3 axis = fabs(w.x) > 0.1f ? (float3)(0.0f, 1.0f, 0.0f) : (float3)(1.0f, 0.0f, 0.0f);
	float3 u = normalize(cross(axis, w));
	float3 v = cross(w, u);
	/* use the coordinte frame and random numbers to compute the next ray direction */
	float3 newdir = normalize(u * cos(rand1)*rand2s + v * sin(rand1)*rand2s + w * sqrt(1.0f - rand2) + reflect_dir * rand2s*shininess);

	return newdir;
}

static int get_Qtable_index(const float3 position) {
	int x_index = (int)((position.x - x_min) / section);
	int y_index = (int)((position.y - y_min) / section);
	int z_index = (int)((position.z - z_min) / section);
	return x_index + y_index * x_size + z_index * y_size * x_size;
}

/* map the input direction into one of the stantard directions */
static int map_direction(const float3 dir) {
	float min = 1000.0;
	int min_index = -1;
	for (int i = 0; i < 26; i++) {
		float dist = length(dir - standard_dirs[i]);
		if (dist < min) {
			min = dist;
			min_index = i;
		}
	}
	return min_index;
}

static int check_pos_valid(const float3 pos) {
	if (pos.x < x_max && pos.x > x_min && pos.y < y_max && pos.y > y_min && pos.z < z_max && pos.z > z_min)
		return 1;
	return 0;
}

static int update_Qtable(__global Qnode* Qtable, float3 next_hit, float3 cur_point, float3 cur_dir, float reward) {
	/* check if position valid*/
	if (check_pos_valid(cur_point) == 0 )
		return -1;
	/* cast to Qtable index and map to standard dir */
	int cur_Q_index = get_Qtable_index(cur_point);
	int next_Q_index = get_Qtable_index(next_hit);
	int dir_index = map_direction(cur_dir);
	/* update with bellman equa */
	if (check_pos_valid(next_hit) == 0) {
		Qtable[cur_Q_index].action[dir_index] = (1 - LR)*(Qtable[cur_Q_index].action[dir_index])
			+ LR * reward;
		Qtable[cur_Q_index].max = max(Qtable[cur_Q_index].max, Qtable[cur_Q_index].action[dir_index]);
		return -1;
	}
	else {
		Qtable[cur_Q_index].action[dir_index] = (1 - LR)*(Qtable[cur_Q_index].action[dir_index])
			+ LR * (reward + GAMMA * Qtable[next_Q_index].max);
		Qtable[cur_Q_index].max = max(Qtable[cur_Q_index].max, Qtable[cur_Q_index].action[dir_index]);
	}
	return next_Q_index;
}


/* the path tracing function (Monte Carlo Version) */
/* return the color thru the tracing process */
/* RL equation used: 
	Q(ray.origin, ray.dir) = (1-LR)Q(ray.origin, ray.dir) + LR(reward(ray.origin) + gamma*max(Q(ray.hit) )*/
float3 trace(__constant Sphere* spheres, const Ray* camray, const int sphere_count, 
			int * seed0, int * seed1, __global Qnode* Qtable){

	Ray ray = *camray;

	float3 accum_color = (float3)(0.0f, 0.0f, 0.0f);

	float t;   /* distance to intersection */
	int hitsphere_id = -1; /* index of intersected sphere */	
	float3 accum_specular = (float3)(1.0f, 1.0f, 1.0f); /* specular of prev objs */
	float reward = 0.0;

	/* start the tracing process */
	for (int i = 0; i < RECURSE_DEPTH; i++){

		/* todo, get the reward on current step (emission of sphere); ignore the first ray from camera*/
		if (hitsphere_id != -1 && length(spheres[hitsphere_id].emission) > 0) {
			/*collect reward on current state*/
			reward = length(spheres[hitsphere_id].emission);
		}
		else reward = 0.0;

		/* shoot the ray */
		bool intersect = intersect_scene(spheres, &ray, &t, &hitsphere_id, sphere_count);
		/* if ray misses scene, return background colour ON THE FIRST RAY*/
		if ( !intersect && i==0) 
			return accum_color += background_color;
		/* if ray miss ON REFLECTION, break the loop */
		else if ( !intersect && i > 0){
			accum_color += background_color*accum_specular;
			break;
		}
		/* else, we've got a hit! Fetch the closest hit sphere */
		Sphere hitsphere = spheres[hitsphere_id]; /* version with local copy of sphere */
		/* compute the hitpoint using the ray equation */
		float3 hitpoint = ray.origin + ray.dir * t;

		int hit_Q_index = -1;
		/* update Q table */
		if(i!=0)
			hit_Q_index = update_Qtable(Qtable, hitpoint, ray.origin, ray.dir, reward);/*q table is updated here*/

		/* compute the surface normal and flip it if necessary to face the incoming ray */
		float3 normal = normalize(hitpoint - hitsphere.pos);
		float3 normal_facing = dot(normal, ray.dir) < 0.0f ? normal : normal * (-1.0f);
		float3 reflect_dir = normalize(ray.dir - 2 * dot(ray.dir,normal_facing) * normal_facing);
		float3 newdir = (0.0);
		/* get the new ray's dir according to the q learning result so far */
		if (hit_Q_index != -1) {
			Qnode node = Qtable[hit_Q_index];
			for (int atmpt = 6; atmpt >= 0; atmpt--) {
				newdir = generate_rand_dir(seed0, seed1, normal_facing, reflect_dir, hitsphere.shininess);
				int mapped_dir_index = map_direction(newdir);
				if (node.action[mapped_dir_index] > 0.75 * node.max)
					break;
			}
		}
		else
			newdir = generate_rand_dir(seed0, seed1, normal_facing, reflect_dir, hitsphere.shininess);

		/* add a very small offset to the hitpoint to prevent self intersection */
		ray.origin = hitpoint + normal_facing * EPSILON;
		ray.dir = newdir;

		/* accumulate color and specular */
		accum_color += hitsphere.emission * accum_specular;
		accum_specular *= hitsphere.specular;
		accum_specular *= dot(newdir, normal_facing);

		/* if reflection gets too low, stop */
		if (length(accum_specular) < SPECULAR_THRESHOLD)
			break;
	}

	return accum_color;
}

union Colour{ float c; uchar4 components; };

__kernel void render_kernel(__constant Sphere* spheres, const int width, const int height, 
	const int sphere_count, __global float3* output,  const int framenumber,__constant const Camera* cam, 
	__global float3* accumbuffer, __global float3* output2, const int num_sample, const int rand0, const int rand1, const int acu_sample,
	const int Qtable_size, __global Qnode* Qtable)
{

	unsigned int work_item_id = get_global_id(0);	/* the unique global id of the work item for the current pixel */
	unsigned int x_coord = work_item_id % width;			/* x-coordinate of the pixel */
	unsigned int y_coord = work_item_id / width;			/* y-coordinate of the pixel */
	
	unsigned int seed0 = x_coord * framenumber % 1000 + (rand0 * 100);
	unsigned int seed1 = y_coord * framenumber % 1000 + (rand1 * 100);

	float3 finalcolor = (float3)(0.0);
	Ray camray = createCamRay(x_coord , y_coord , width, height, cam);
	for (int i = 0; i<num_sample; i++){
		finalcolor +=  trace(spheres, &camray, sphere_count, &seed0, &seed1, Qtable) *1.0f/ num_sample;
	}
	float3 tempcolor = finalcolor;

	/* skip update when framenumber more than 1 and does not need to acumulate sample*/
	if(framenumber>1 && acu_sample ==0)
		return;

	/* reset acum buffer */
	else if(framenumber == 1){
		accumbuffer[work_item_id] = finalcolor;
	}

	/* add pixel colour to accumulation buffer (accumulates all samples) */
	else{
		accumbuffer[work_item_id] = accumbuffer[work_item_id] + (finalcolor - accumbuffer[work_item_id])/framenumber;
		tempcolor = accumbuffer[work_item_id]; 
	}

	/* clamp the color if more than 1 */
	float max_color = max(tempcolor.x,tempcolor.y);
	max_color = max(max_color,tempcolor.z);
	if(max_color >= 1){
		tempcolor.x /= max_color;
		tempcolor.y /= max_color;
		tempcolor.z /= max_color;
	}

	/*debug
	float3 tt = (0.0);
	tt.x = 5.99f; tt.y = 2.99f; tt.z = 5.99f;
	Qtable[2].max = get_Qtable_index(tt);*/

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
