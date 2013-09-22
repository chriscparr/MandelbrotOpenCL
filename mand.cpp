#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <CL/cl.h>
#define MAX_SOURCE_SIZE (0x100000) //1048576 chars is more than enough to hold the kernel source code. 

struct params{
		int width;
		int height;
		double max_x;
		double max_y;
		double min_x;
		double min_y;
		int iter;
		int frames;
};

int main(int argc, char **argv){
  struct params p = {640,480,0,1.5,-3,-1.5,300,600};	//default test case
  size_t localworksize = 300;				//default work size (# of images calculated in each batch)
	int c;
	opterr = 0;
	while((c = getopt(argc,argv,"f:w:i:")) != -1)
	switch (c){
		case 'f':
			if(atoi(optarg)>0){
			p.frames = atoi(optarg);		//process args
			}
			break;
		case 'w':
			if(atoi(optarg)>0){
			localworksize = atoi(optarg);
			}
			break;
		case 'i':
			p.iter = atoi(optarg);
			break;
		case '?':
			ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms); //get platform & openCL device(s) information
			ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices); //choose the 1st device, usually the only one 
			
			//how much mem do we have on openCL device?
			cl_ulong max_alloc;
        		ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &max_alloc, NULL);

			fprintf(stderr,"\nMaximum device memory allocation = %ld\n",max_alloc);
			cl_ulong work_size = (max_alloc/(p.width*p.height*3));	//maximum work_size our device can handle
			fprintf("The work size should ideally be: %ld\n",work_size);
			fprintf(stderr,"\nCheck that you have enough RAM on the host to handle this\n");
			return 1;
	}


  cl_device_id device_id = NULL;
  cl_context context = NULL;
  cl_command_queue command_queue = NULL;
  cl_mem memobj = NULL;
  cl_program program = NULL;
  cl_kernel kernel = NULL;
  cl_platform_id platform_id = NULL;
  cl_uint ret_num_devices;
  cl_uint ret_num_platforms;
  cl_int ret;
  int fd;
  char buffer[100];
  
  

  char *pic = (char*)malloc(p.width*p.height*3*localworksize*sizeof(char));		//alloc mem on host for 1 block of the resulting frames, this is reused for each round


  FILE *fp;
  char fileName[] = "./mand.cl";
  char *source_str;
  size_t source_size;

  /* Load the source code containing the kernel*/
  fp = fopen(fileName, "r");
  if (!fp) {
    fprintf(stderr, "Failed to load kernel.\n");
    exit(1);
  }
  source_str = (char*)malloc(MAX_SOURCE_SIZE);
  source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  /* Get Platform and Device Info */
  ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
  ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

  /* Create OpenCL context */
  context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);

  /* Create Command Queue */
  command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

  /* Create Memory Buffer */
  memobj = clCreateBuffer(context, CL_MEM_READ_WRITE,(p.width * p.height * 3 * localworksize * sizeof(char)), NULL, &ret);

  /* Create Kernel Program from the source */
  program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &ret);

  /* Build Kernel Program */
  ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

  /* Create OpenCL Kernel */
  kernel = clCreateKernel(program, "mand", &ret);



int rounds = p.frames/localworksize;
int remain = p.frames%localworksize;
if(remain > 0)
	rounds++;	//This ensures that the remainder of the frames are rendered
int i;
for(i=0;i<rounds;i++){
  /* Set OpenCL Kernel Arguments */
  ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memobj); //pass kernel args

  ret = clSetKernelArg(kernel, 1, sizeof(int), &i);

  ret = clSetKernelArg(kernel, 2, sizeof(struct params), &p);

  /* Execute OpenCL Kernel */
  ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &localworksize, &localworksize, 0, NULL, NULL);

  /* Copy results from the memory buffer */
  ret = clEnqueueReadBuffer(command_queue, memobj, CL_TRUE, 0,
			                (p.height * p.width * 3 * localworksize * sizeof(char)),pic, 0, NULL, NULL);
  /* Display Result */
  /* writes the data to a TGA file */
int b;
char filename[12];
for(b = 0;b<localworksize;b++){
  sprintf(filename,"%d.tga",(b+(i*localworksize)));

  char *pnt = pic + (p.width*p.height*3*b*sizeof(char));	//points to the beginning of the frame

  if ((fd = open(filename, O_RDWR+O_CREAT, 0751)) == -1)
  {
    printf("error opening file: %s\n",filename);
    exit(1);
  }
  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = 2;
  buffer[8] = 0; buffer[9] = 0;
  buffer[10] = 0; buffer[11] = 0;
  buffer[12] = (p.width & 0x00FF); buffer[13] = (p.width & 0xFF00) >> 8;
  buffer[14] = (p.height & 0x00FF); buffer[15] = (p.height & 0xFF00) >> 8;
  buffer[16] = 24;
  buffer[17] = 0;
  write(fd, buffer, 18);
  write(fd, pnt, p.width*p.height*3);
  close(fd);
}
}

  /* Finalization */
  ret = clFlush(command_queue);
  ret = clFinish(command_queue);
  ret = clReleaseKernel(kernel);
  ret = clReleaseProgram(program);
  ret = clReleaseMemObject(memobj);
  ret = clReleaseCommandQueue(command_queue);
  ret = clReleaseContext(context);

  free(source_str);
  free(pic);
  return 0;
}


