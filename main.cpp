// Ray Tracing program
// -Chunan Huang
// Adpadted from the:
//	OpenCL ray tracing tutorial by Sam Lapere, 2016
//	http://raytracey.blogspot.com

#include <iostream>
#include <fstream>
#include <vector>
#include "cl_gl_interop.h"
#include <CL\cl.hpp>
#include <windows.h>
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

// get from Scene.cpp
const int sphere_count = sphere_num;
const int light_count = light_num;
const int fps = 30;
const string img_dir = "./ScreenShot/";

// OpenCL objects
Device device;
CommandQueue queue;
Kernel kernel;
Context context;
Program program;
Buffer cl_spheres;
Buffer cl_lights;
Buffer cl_camera;
Buffer cl_accumbuffer;
BufferGL cl_vbo;
vector<Memory> cl_vbos;
bool save_img;
BYTE * pixels; // used for save the image
FIBITMAP *img;
int img_num=0;

cl_float4 * cpu_output;
Buffer cl_output;

cl_int err;

Camera* hostRendercam = NULL;
Sphere cpu_spheres[sphere_count];
Light cpu_lights[light_count];

/* -------------------------------------------------------------------------- */
//	The following functions are used to setup the OpenCL environment
/* -------------------------------------------------------------------------- */

// function that allows user to pick OpenCL running platform
void pickPlatform(Platform& platform, const vector<Platform>& platforms) {

	if (platforms.size() == 1) platform = platforms[0];
	else {
		int input = 0;
		cout << "\nChoose platform: ";
		cin >> input;

		// handle incorrect user input
		while (input < 1 || input > platforms.size()) {
			cin.clear(); //clear errors/bad flags on cin
			cin.ignore(cin.rdbuf()->in_avail(), '\n'); // ignores exact number of chars in cin buffer
			cout << "ERR. Choose an OpenCL platform: ";
			cin >> input;
		}
		platform = platforms[input - 1];
	}
}

// function that allows user to pick OpenCL running device
void pickDevice(Device& device, const vector<Device>& devices) {

	if (devices.size() == 1) device = devices[0];
	else {
		int input = 0;
		cout << "\nChoose a device: ";
		cin >> input;

		// handle incorrect user input
		while (input < 1 || input > devices.size()) {
			cin.clear(); //clear errors/bad flags on cin
			cin.ignore(cin.rdbuf()->in_avail(), '\n'); // ignores exact number of chars in cin buffer
			cout << "ERR. Choose an OpenCL device: ";
			cin >> input;
		}
		device = devices[input - 1];
	}
}

void printErrorLog(const Program& program, const Device& device) {

	// Get the error log and print to console
	string buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
	cerr << "Build log:" << std::endl << buildlog << std::endl;

	// Print the error log to a file
	FILE *log = fopen("errorlog.txt", "w");
	fprintf(log, "%s\n", buildlog);
	cout << "Error log saved in 'errorlog.txt'" << endl;
	system("PAUSE");
	exit(1);
}

void initOpenCL()
{
	// Get all available OpenCL platforms (e.g. AMD OpenCL, Nvidia CUDA, Intel OpenCL)
	vector<Platform> platforms;
	Platform::get(&platforms);
	cout << "Available OpenCL platforms : " << endl << endl;
	for (int i = 0; i < platforms.size(); i++)
		cout << "\t" << i + 1 << ": " << platforms[i].getInfo<CL_PLATFORM_NAME>() << endl;

	// Pick one platform
	Platform platform;
	pickPlatform(platform, platforms);
	cout << "\nUsing OpenCL platform: \t" << platform.getInfo<CL_PLATFORM_NAME>() << endl;

	// Get available OpenCL devices on platform
	vector<Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

	cout << "Available OpenCL devices on this platform: " << endl << endl;
	for (int i = 0; i < devices.size(); i++) {
		cout << "\t" << i + 1 << ": " << devices[i].getInfo<CL_DEVICE_NAME>() << endl;
		cout << "\t\tMax compute units: " << devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << endl;
		cout << "\t\tMax work group size: " << devices[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << endl << endl;
	}

	// Pick one device
	//Device device;
	pickDevice(device, devices);
	cout << "\nUsing OpenCL device: \t" << device.getInfo<CL_DEVICE_NAME>() << endl;
	cout << "\t\t\tMax compute units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << endl;
	cout << "\t\t\tMax work group size: " << device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << endl;

	// Create an OpenCL context on that device.
	// Windows specific OpenCL-OpenGL interop
	cl_context_properties properties[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
		0
	};

	context = Context(device, properties);

	// Create a command queue
	queue = CommandQueue(context, device);


	// Convert the OpenCL source code to a string// Convert the OpenCL source code to a string
	string source;
	ifstream file("Source_Files/opencl_kernel.cl");
	if (!file) {
		cout << "\nNo OpenCL file found!" << endl << "Exiting..." << endl;
		system("PAUSE");
		exit(1);
	}
	while (!file.eof()) {
		char line[256];
		file.getline(line, 255);
		source += line;
	}

	const char* kernel_source = source.c_str();

	// Create an OpenCL program with source
	program = Program(context, kernel_source);

	// Build the program for the selected device 
	cl_int result = program.build({ device }); // "-cl-fast-relaxed-math"
	if (result) cout << "Error on compilation of OpenCL code!!!\n (" << result << ")" << endl;
	if (result == CL_BUILD_PROGRAM_FAILURE) printErrorLog(program, device);
}


/* -------------------------------------------------------------------------- */
//	The following functions are used to set the OpenCL kernel
/* -------------------------------------------------------------------------- */

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
}

inline float clamp(float x){ return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }
// with garma correction
inline int toInt(float x){ return int( 255* pow(clamp(x), 1.0/1.0) ); }

void runKernel(bool save_img) {
	// every pixel in the image has its own thread or "work item",
	// so the total amount of work items equals the number of pixels
	std::size_t global_work_size = window_width * window_height;
	std::size_t local_work_size = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);;

	// Ensure the global work size is a multiple of local work size
	if (global_work_size % local_work_size != 0)
		global_work_size = (global_work_size / local_work_size + 1) * local_work_size;

	//Make sure OpenGL is done using the VBOs
	glFinish();

	//this passes in the vector of VBO buffer objects 
	queue.enqueueAcquireGLObjects(&cl_vbos);
	queue.finish();

	// launch the kernel
	queue.enqueueNDRangeKernel(kernel, NULL, global_work_size, local_work_size); // local_work_size
	queue.finish();

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

	// refresh
	glutTimerFunc(1000.0/fps, update_frame, 0);

}

void cleanUp() {
	delete cpu_output;
}

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
