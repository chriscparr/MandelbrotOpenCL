#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

struct params{
		int width;
		int height;
		double max_x;
		double max_y;	//struct for initial values
		double min_x;	//passed from host
		double min_y;
		int iter;
		int frames;
};

int offset(int h, int w, int z, struct params p);

__kernel void mand(__global char* pic, int round, struct params p) //passing in the pointer to main mem,
{
  double x,y;
  double xstart;
  double xstep;
  double ystart;
  double ystep;
  double xend, yend;
  double z,zi,newz,newzi;
  double colour;
  long col;
  int i,j,k;
  int inset;
  int fd;

int id = get_local_id(0) + (round * get_local_size(0));
//given a frame number, calc frame coords...

double xStep = (p.max_x - p.min_x)/((p.frames * 2)+1);
double yStep = (p.max_y - p.min_y)/((p.frames * 2)+1);

xend = p.max_x - (xStep * id);
yend = p.max_y - (yStep * id);
xstart = p.min_x + (xStep * id);
ystart = p.min_y + (yStep * id);

/* these are used for calculating the points corresponding to the pixels */
  xstep = (xend-xstart)/p.width;
  ystep = (yend-ystart)/p.height;

  /*the main loop */
  x = xstart;
  y = ystart;


  for (i=0; i<p.height; i++)	//for each row of image
  {
    for (j=0; j<p.width; j++)	//for each column within the row
    {
      z = 0;
      zi = 0;
      inset = 1;
      for (k=0; k<p.iter; k++)	//iterate through the m-set function 
      {
        /* z^2 = (a+bi)(a+bi) = a^2 + 2abi - b^2 */
	newz = (z*z)-(zi*zi) + x;
	newzi = 2*z*zi + y;
        z = newz;
        zi = newzi;
	if(((z*z)+(zi*zi)) > 4)	//if the bailout is reached
	{
	  inset = 0;	
	  colour = k;
	  k = p.iter;
	}
      }
      if (inset == 1)	//for all points within the set
      {
	pic[offset(i,j,0,p) + get_local_id(0)*p.height*p.width*3] = 0;	//b
	pic[offset(i,j,1,p) + get_local_id(0)*p.height*p.width*3] = 0;	//g
	pic[offset(i,j,2,p) + get_local_id(0)*p.height*p.width*3] = 0;	//r
      }
      else
      {
	int index = colour; //for all other points, colour them appropriately
	if(colour > 255){
		index = 255;}

	pic[offset(i,j,0,p) + get_local_id(0)*p.height*p.width*3] =  colour / p.iter * 255;  	 //b
	pic[offset(i,j,1,p) + get_local_id(0)*p.height*p.width*3] =  colour / p.iter * 255 / 2;  //g
	pic[offset(i,j,2,p) + get_local_id(0)*p.height*p.width*3] =  colour / p.iter * 255 / 2;  //r

      }
      x += xstep;
    }
    y += ystep;
    x = xstart;
  }
}
//memory offset function. replaces 3D array
int offset(int h, int w, int z, struct params p){
return ((h * p.width * 3) + (w * 3) + z);
}
