// Ray Tracing program
// -Chunan Huang
// Adpadted from the:
//	OpenCL ray tracing tutorial by Sam Lapere, 2016
//	http://raytracey.blogspot.com
#include <iostream>
#include <fstream>
#include <vector>
#include "setup.cpp"
#include "linear_algebra.h"
#include "camera.h"
#include "geometry.h"
#include "user_interaction.h"
#include "Scene.cpp"
#include <FreeImage.h>
#include <math.h>
#include <string>

using namespace std;
using namespace cl;

const int fps = 40;
const int fps_check_rate = 30; // print fps every 30 frames

const int pixel_skip = 0; // skip some pixel when render

// get from Scene.cpp
const int sphere_count = sphere_num;
const int light_count = light_num;

// !!! resolution of window is defined in cl_gl_interop.h

// scene object variable
Buffer cl_spheres;
Buffer cl_lights;
Buffer cl_camera;
Buffer cl_accumbuffer;
BufferGL cl_vbo;
Camera* hostRendercam = NULL;
Sphere cpu_spheres[sphere_count];
Light cpu_lights[light_count];

vector<Memory> cl_vbos;
cl_float4 * cpu_output;
Buffer cl_output;
cl_int err;

// image-saving variable
bool save_img;
BYTE * pixels; // used for save the image
FIBITMAP *img;
int img_num=0;
const string img_dir = "./ScreenShot/";

// timer
std::clock_t start;
std::clock_t start_kernel;
int num_render = 0;
double sum_duration = 0; 
double sum_duration_kernel = 0; 

/* -------------------------------------------------------------------------- */
//	The following functions are used to setup the OpenCL environment
/* -------------------------------------------------------------------------- */
void pickPlatform(Platform& platform, const vector<Platform>& platforms);
void pickDevice(Device& device, const vector<Device>& devices);
void printErrorLog(const Program& program, const Device& device);
void initOpenCL();

// init the kernel program; set init parameter
void initCLKernel() {

	// Create a kernel (entry point in the OpenCL source program)
	kernel = Kernel(program, "render_kernel");

	// specify OpenCL kernel arguments
	kernel.setArg(0, cl_spheres);
	kernel.setArg(1, window_width);
	kernel.setArg(2, window_height);
	kernel.setArg(3, sphere_count);
	kernel.setArg(4, cl_vbo);
	kernel.setArg(5, 0);
	kernel.setArg(6, cl_camera);
	kernel.setArg(7, cl_accumbuffer);
	kernel.setArg(8, cl_lights);
	kernel.setArg(9, light_count);
	kernel.setArg(10, cl_output);
	kernel.setArg(11, pixel_skip+1);
}

inline float clamp(float x){ return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }
inline int toInt(float x){ return int( 255* clamp(x) ); }

void runKernel(bool save_img) {
	// every pixel in the image has its own thread or "work item",
	// so the total amount of work items equals the number of pixels
	std::size_t global_work_size = window_width * window_height ;
	std::size_t local_work_size = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);;

	// Ensure the global work size is a multiple of local work size
	if (global_work_size % local_work_size != 0)
		global_work_size = (global_work_size / local_work_size + 1) * local_work_size;

	//Make sure OpenGL is done using the VBOs
	glFinish();

	//this passes in the vector of VBO buffer objects 
	queue.enqueueAcquireGLObjects(&cl_vbos);
	queue.finish();

	// run the kernel 
	if(checkFPS)
		start_kernel = std::clock();
	// launch the kernel
	queue.enqueueNDRangeKernel(kernel, NULL, global_work_size, local_work_size); // local_work_size
	queue.finish();
	// check the kernel running time
	if(checkFPS){
		if (num_render == fps_check_rate){
			cout << "Current time of calculation/frame is: " << (sum_duration_kernel/num_render) << endl; 
			sum_duration_kernel = 0;
		}
		else
			sum_duration_kernel += ( std::clock() - start_kernel ) / (double) CLOCKS_PER_SEC;
	}

	// save the image
	if(save_img){
		// first get the buffer data to cpu
		queue.enqueueReadBuffer(cl_output, CL_TRUE, 0, window_width * window_height * sizeof(cl_float3), cpu_output);
		
		// fill the pixels byte
		for (int i = 0; i < window_height; i++) {
			for (int j = 0; j < window_width; j++) {
					int start = (i * window_width + (window_width-1-j)) * 3;
					int cpu_output_index = window_height*window_width -1 -(i * window_width + j);
					pixels[start] = toInt(cpu_output[cpu_output_index].s[2]);
					pixels[start + 1] = toInt(cpu_output[cpu_output_index].s[1]);
					pixels[start + 2] = toInt(cpu_output[cpu_output_index].s[0] );	
			}
		}
		img = FreeImage_ConvertFromRawBits(pixels, window_width, window_height, window_width * 3, 24, 0xFF0000, 0x00FF00, 0x0000FF, true);
		string img_name = img_dir + "screen_shot_" + to_string(img_num++)+ ".png";
		char img_name_char[200];
		strcpy(img_name_char, img_name.c_str());
		FreeImage_Save(FIF_PNG, img, img_name_char, 0);
		cout<< "Image saved to " << img_name<<endl;
	}
	
	//Release the VBOs so OpenGL can play with them
	queue.enqueueReleaseGLObjects(&cl_vbos);
	queue.finish();
}

// call back for updating frame
void update_frame(int value) {
	glutPostRedisplay();
}

// function called to render everyframe
void render() {

	if(checkFPS)
		start = std::clock();

	if (buffer_reset) {
		float arg = 0;
		queue.enqueueFillBuffer(cl_accumbuffer, arg, 0, window_width * window_height * sizeof(cl_float3));
		interactiveCamera->framenumber = 0;
	} 

	buffer_reset = false;
	interactiveCamera->framenumber++;

	// build a new camera for each frame on the CPU
	interactiveCamera->buildRenderCamera(hostRendercam);
	// copy the host camera to a OpenCL camera
	queue.enqueueWriteBuffer(cl_camera, CL_TRUE, 0, sizeof(Camera), hostRendercam);
	queue.finish();

	kernel.setArg(5, interactiveCamera->framenumber);
	kernel.setArg(6, cl_camera);

	// delegate the kernel to run the rendering process
	save_img = interactiveCamera->save_img;
	interactiveCamera->save_img = false;

	runKernel(save_img);
	drawGL();

	if(checkFPS){
		if (num_render == fps_check_rate){
			cout << "Current FPS is: " << 1.0/(sum_duration/num_render) << endl; 
			sum_duration = 0;
			num_render = 0;
		}
		else{
			sum_duration += ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
			num_render++;
		}
	}

	// refresh
	glutTimerFunc(1000.0/fps, update_frame, 0);

}

void cleanUp() {
	delete cpu_output;
}

void check_fps();

// initialise camera on the CPU
void initCamera()
{
	delete interactiveCamera;
	interactiveCamera = new InteractiveCamera();

	interactiveCamera->setResolution(window_width, window_height);
	interactiveCamera->setFOVX(45);
}

void main(int argc, char** argv) {

	// initialise OpenGL (GLEW and GLUT window + callback functions)
	initGL(argc, argv);
	cout << "OpenGL initialized \n";

	// initialise OpenCL to select platform/device
	initOpenCL();

	// ask if user wanna check FPS
	check_fps();

	// create vertex buffer object
	createVBO(&vbo);

	cpu_output = new cl_float3[window_width * window_height];
	cl_output = Buffer(context, CL_MEM_WRITE_ONLY, window_width * window_height * sizeof(cl_float3));

	//make sure OpenGL is finished before we proceed
	glFinish();

	// initialise scene
	initScene(cpu_spheres, cpu_lights);
	FreeImage_Initialise();
	pixels = new BYTE[3 * window_width*window_height];

	// scene buffer
	cl_spheres = Buffer(context, CL_MEM_READ_ONLY, sphere_count * sizeof(Sphere));
	cl_lights = Buffer(context, CL_MEM_READ_ONLY, light_count * sizeof(Light));
	queue.enqueueWriteBuffer(cl_spheres, CL_TRUE, 0, sphere_count * sizeof(Sphere), cpu_spheres);
	queue.enqueueWriteBuffer(cl_lights, CL_TRUE, 0, light_count * sizeof(Light), cpu_lights);

	// initialise an interactive camera on the CPU side
	initCamera();
	// create a CPU camera
	hostRendercam = new Camera();
	interactiveCamera->buildRenderCamera(hostRendercam);

	cl_camera = Buffer(context, CL_MEM_READ_ONLY, sizeof(Camera));
	queue.enqueueWriteBuffer(cl_camera, CL_TRUE, 0, sizeof(Camera), hostRendercam);

	// create OpenCL buffer from OpenGL vertex buffer object
	// in this case, changing cl_vob also change the opengl buffer - vbo
	cl_vbo = BufferGL(context, CL_MEM_WRITE_ONLY, vbo);
	cl_vbos.push_back(cl_vbo);

	// reserve memory buffer on OpenCL device to hold image buffer for accumulated samples
	cl_accumbuffer = Buffer(context, CL_MEM_WRITE_ONLY, window_width * window_height * sizeof(cl_float3));

	// intitialise the kernel
	initCLKernel();

	// start rendering continuously
	glutMainLoop();

	// release memory
	cleanUp();
	FreeImage_DeInitialise();
	system("PAUSE");
}
