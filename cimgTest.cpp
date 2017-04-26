#include "CImg.h"
#include <fstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

using namespace std;
using namespace cimg_library;

#define input_file  "input.raw"
#define output_file "output.raw"

//Returns 3D Array of YCbCr Values for each pixel with [height][width][YCbCr (Y - 0, Cb - 1, Cr -2]
int*** getYCbCr(string filename) {
	CImg<unsigned char> img1(filename.c_str());
	int width = img1.width();
    int height = img1.height();

	int ***yCbCrArray;

	yCbCrArray = new int**[height];
	for (int i = 0; i < height; ++i) {
		yCbCrArray[i] = new int*[width];

		for (int j = 0; j < width; ++j) {
			yCbCrArray[i][j] = new int[3];
		}
	}

	CImg<unsigned char> img2;
    img2 = img1.get_RGBtoYCbCr();

    
	for (int r = 0; r < height; r++) {
		for (int c = 0; c < width; c++) {
			for (int i = 0; i < 3; i++) {
	    		yCbCrArray[r][c][i] = (int)img2(r,c,0,i);
	    	}
		}
	}

	return yCbCrArray;
}


int main(int argc, char** argv) 
{
	int ***image = getYCbCr("input.jpg");

	CImg<unsigned char> img1("input.jpg");
	int width = img1.width();
	int height = img1.height();

	//cout << yCbCrArray[0][5][2] << endl;
	
	// measure the start time here
	struct timespec start, stop; 
	double time;

	if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}

  //Mico, Kyle Start
	FILE* fp;
  int  dev = 255, alpha = .5, X_CONSTANT=128; //NEED TO SET
  int Cr = 2, Cb = 1, Y=0;
  int background = Cr;
  int i,j,k; //loop variables
  
  if(image[0][0][Cb] >= 128)
  {
    background = Cb;
  }
  
  int bg_centroid = image[0][0][background];
  int fg_centroid = bg_centroid + dev;
  int unk_centroid = alpha* abs(bg_centroid - fg_centroid);

  unsigned char **cluster = (unsigned char**) malloc (sizeof(unsigned char*)*3);
  for (i = 0; i < 3; ++i) {
	cluster[i] = (unsigned char*) malloc (sizeof(unsigned char)*height*width);
  }

  unsigned char **unk_points = (unsigned char**) malloc (sizeof(unsigned char*)*height*width);
  for (i = 0; i < height * width; ++i) {
	unk_points[i] = (unsigned char*) malloc (sizeof(unsigned char)*2);
  }

  unsigned char **fg_points = (unsigned char**) malloc (sizeof(unsigned char*)*height*width);
  for (i = 0; i < height * width; ++i) {
	fg_points[i] = (unsigned char*) malloc (sizeof(unsigned char)*2);
  }
  unsigned char **bg_points = (unsigned char**) malloc (sizeof(unsigned char*)*height*width);
  for (i = 0; i < height * width; ++i) {
	bg_points[i] = (unsigned char*) malloc (sizeof(unsigned char)*2);
  }

  int bg_i = 0, fg_i =1, unk_i = 2;
  int bg_count = 0, fg_count = 0, unk_count = 0;


  for(k = 0; k < 50; k++)
  {
  	bg_count = fg_count = unk_count = 0;
  	//initial bucket allocation	
    for(i = 0; i< height; i++)
    {
      for(j =0; j<width; j++)
      {
        unsigned char cur_pixel = image[i][j][background];
        unsigned char distance_bg = cur_pixel - bg_centroid;
        unsigned char distance_fg = cur_pixel - fg_centroid;
        unsigned char distance_unk = cur_pixel - unk_centroid;
			
        if(distance_bg < distance_fg && distance_bg < distance_unk)
        {
        	cluster[bg_centroid][bg_count] = cur_pixel;
        	bg_points[bg_count][0] = i;
        	bg_points[bg_count][1] = j;
        	bg_count++;
        }
        else if(distance_fg < distance_bg && distance_fg <  distance_unk)
        {
        	cluster[fg_centroid][fg_count] = cur_pixel;
        	fg_points[fg_count][0] = i;
        	fg_points[fg_count][1] = j;
        	fg_count++;
        }
        else
        {
        	cluster[unk_centroid][unk_count] = cur_pixel;
        	unk_points[unk_count][0] = i;
        	unk_points[unk_count][1] = j;
        	unk_count++;
        }
      }
    }

    //recalculate averages
    unsigned char sum = 0;
    for(i = 0; i < bg_count; i++)
    {
    	unsigned char cur = cluster[bg_centroid][i];
    	sum += cur;
    }
    bg_centroid = sum/bg_count;

    sum = 0;
    for(i = 0; i < fg_count; i++)
    {
    	unsigned char cur = cluster[fg_centroid][i];
    	sum += cur;
    }
    fg_centroid = sum/fg_count;

    unk_centroid = alpha* abs(bg_centroid - fg_centroid);

  }

  //UNKNOWN
  unsigned char *region = (unsigned char*) malloc (sizeof(unsigned char*)*25);

  unsigned char B_max = cluster[bg_i][0]; 
  unsigned char F_max = cluster[fg_i][0];
  unsigned char B_min = cluster[bg_i][0]; 
  unsigned char F_min = cluster[fg_i][0];

  for(i = 0; i< bg_count; i++)
  {
  	if( cluster[bg_i][i] < B_max) B_max = cluster[bg_i][i];

  	if( cluster[bg_i][i] > B_min) B_min = cluster[bg_i][i];
  }

  for(i =0; i<fg_count; i++)
  {
  	if( cluster[fg_i][i] < F_max) F_max = cluster[fg_i][i];

  	if( cluster[fg_i][i] > F_min) F_min = cluster[fg_i][i];
  }

  //Unknown bucket allocation
  for(i = 0; i < unk_count ; i++)
  {
  	unsigned char cur_unk_pixel = cluster[unk_i][i];
  	unsigned char x = unk_points[i][0];
  	unsigned char y = unk_points[i][1];

  	for(j = 0; j < 5; j++)
  	{
  		for( k = 0; k < 5; k++)
  		{
  			if(x-2+j < 0 && y-2+k <0) region[j+k] = image[0][0][background];
  			else if(x-2+j >= height && y-2+k <0 ) region[j+k] = image[height-1][0][background];
  			else if(x-2+j < 0 && y-2+k >= width) region[j+k] = image[0][width -1][background];		
  			else if(x-2+j < 0) region[j+k] = image[0][y-2+k][background];
  			else if(y-2+k < 0) region[j+k] = image[x-2+j][0][background];
  			else if(x-2+j >= height && y-2+k >= width) region[j+k] = image[height-1][width -1][background];
  			else if(x-2+j >= height) region[j+k] = image[height-1][y-2+k][background];
   			else if(y-2+k >= width) region[j+k] = image[x-2+j][width - 1][background];
 			else region[j+k] = image[x-2+j][y-2+j][background];
  		}
  	}

  	//Bn and Fn calculations
  	unsigned char Bn, Fn;
  	if( B_max - B_min > X_CONSTANT)
  	{
  		Bn = bg_centroid;
  	}
  	else
  	{
  		Bn = B_max;
  	}

  	if( F_max - F_min > X_CONSTANT)
  	{
  		Fn = fg_centroid;
  	}
  	else
  	{
  		Fn = F_max;
  	}

  	//Assign UNKOWN 
  	if(abs(Fn - cur_unk_pixel) < abs(cur_unk_pixel - Bn))
  	{
  		cluster[fg_i][fg_count] = cur_unk_pixel;
        fg_points[fg_count][0] = x;
        fg_points[fg_count][1] = y;
  		fg_count++;
  	}
  	else
  	{
   		cluster[bg_i][bg_count] = cur_unk_pixel;
        bg_points[bg_count][0] = x;
        bg_points[bg_count][1] = y;
  		bg_count++; 		
  	}
  }

	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) { perror("clock gettime");}		
		time = (stop.tv_sec - start.tv_sec)+ (double)(stop.tv_nsec - start.tv_nsec)/1e9;

		// print out the execution time here
	printf("Execution time = %f sec\n", time);		
	

	//OUTPUT
	if (!(fp=fopen(output_file,"wb"))) {
		printf("can not open file\n");
		return 1;
	}	
  	unsigned char **output = (unsigned char**) malloc (sizeof(unsigned char*)*height*width);
  	for(i = 0; i<fg_count; i++)
  	{
  		int x = fg_points[i][0];
  		int y = fg_points[i][1];
  		output[x][y] = 1;
  	}

  	for(i = 0; i<bg_count; i++)
  	{
  		int x = bg_points[i][0];
  		int y = bg_points[i][1];
  		output[x][y] = 0;
  	}

	fwrite(output, sizeof(unsigned char),width*height, fp);
	fclose(fp);


  //END
	return 0;
}
