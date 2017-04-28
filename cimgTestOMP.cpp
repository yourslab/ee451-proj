#include "CImg.h"
#include <fstream>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <cmath> 
#include <iomanip>
#include <sstream>

using namespace std;
using namespace cimg_library;

#define input_file  "input.raw"
#define output_file "output.txt"

#define SCHEDULE static
#define THREADS 1
#define BLOCK_SIZE 128

//Returns 3D Array of YCbCr Values for each pixel with [height][width][YCbCr (Y - 0, Cb - 1, Cr -2]
int*** getYCbCr(CImg<unsigned char> img1) {
	int width = img1.width();
	int height = img1.height();

	int ***yCbCrArray;

	yCbCrArray = new int**[height];
	#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
	for (int i = 0; i < height; ++i) {
		yCbCrArray[i] = new int*[width];
		for (int j = 0; j < width; ++j) {
			yCbCrArray[i][j] = new int[3];
		}
	}

	ofstream myfile;
	myfile.open ("example.txt");

	CImg<unsigned char> img2;
	img2 = img1.get_RGBtoYCbCr();

	#pragma omp parallel for collapse(3) num_threads(THREADS) schedule(SCHEDULE)
	for (int r = 0; r < height; r++) {
		for (int c = 0; c < width; c++) {
			for (int i = 0; i < 3; i++) {
				yCbCrArray[r][c][i] = (int)img2(c,r,0,i);
			}
		}
	}
/*
	CImgDisplay main_disp(img2,"Click a point");
	while (!main_disp.is_closed()) {
    	main_disp.wait();
	}
*/
	return yCbCrArray;
}


int main(int argc, char** argv) 
{
	const string inputObjectName[] = {"flipphones", "forks", "hammers", "mugs", "pliers", "scissors", "staplers", "telephones", "watches"};
	string filename;
	CImg<unsigned char> img1;
	double totalTime = 0.0f;

	for (int objectIndex = 0; objectIndex < 9; objectIndex++) {
		for (int imageIndex = 0; imageIndex < 20; imageIndex++) {
			std::ostringstream ss;
			ss << "images/" << inputObjectName[objectIndex] << "." << setfill('0') << setw(3) << imageIndex << ".jpg";
			filename = ss.str();
			cout << "Filename: " << filename << endl;

			img1.load_jpeg(filename.c_str());
			cout << imageIndex << endl;
			int ***image = getYCbCr(img1);

			int width = img1.width();
			int height = img1.height();
			
			// measure the start time here
			struct timespec start, stop; 
			double time;

			if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}

			//Mico, Kyle Start
			FILE* fp;
			int  dev = 40, X_CONSTANT =128;
			double alpha = 0.5; //NEED TO SET
			int Cr = 2, Cb = 1, Y=0;
			int background = Cr;
			int i,j,k; //loop variables
			
			if(image[0][0][Cb] >= 128)
			{
				background = Cb;
			}
			
			double bg_centroid = image[0][0][background];
			double fg_centroid = bg_centroid + dev;
			double distance_unk = alpha*abs(bg_centroid - fg_centroid);

			double bg_centroid_prev = 0;
			double fg_centroid_prev = 0;

			int **cluster = (int**) malloc (sizeof(int*)*3);
			#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
			for (i = 0; i < 3; ++i) {
				cluster[i] = (int*) malloc (sizeof(int)*height*width);
			}

			int **unk_points = (int**) malloc (sizeof(int*)*height*width);
			#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
			for (i = 0; i < height * width; ++i) {
				unk_points[i] = (int*) malloc (sizeof(int)*2);
			}

			int **fg_points = (int**) malloc (sizeof(int*)*height*width);
			#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
			for (i = 0; i < height * width; ++i) {
				fg_points[i] = (int*) malloc (sizeof(int)*2);
			}
			int **bg_points = (int**) malloc (sizeof(int*)*height*width);
			#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
			for (i = 0; i < height * width; ++i) {
				bg_points[i] = (int*) malloc (sizeof(int)*2);
			}

			int bg_i = 0, fg_i =1, unk_i = 2;
			int bg_count = 0;
			int fg_count = 0;
			int unk_count = 0;

			int num_iters = 1;

			for(int k = 0; k < num_iters; k++)
			{

				bg_count = 0;
				fg_count = 0;
				unk_count = 0;
				//initial bucket allocation
				for(i = 0; i< height; i++)
				{
					for(j =0; j<width; j++)
					{
						int cur_pixel = image[i][j][background];
						int distance_bg = pow(cur_pixel - bg_centroid, 2.0);
						int distance_fg = pow(cur_pixel - fg_centroid, 2.0);

						if(abs(distance_bg - distance_fg) < distance_unk) {
							cluster[unk_i][unk_count] = cur_pixel;
							unk_points[unk_count][0] = i;
							unk_points[unk_count][1] = j;
							unk_count++;
						}
						else if(distance_bg < distance_fg)
						{
							cluster[bg_i][bg_count] = cur_pixel;
							bg_points[bg_count][0] = i;
							bg_points[bg_count][1] = j;
							bg_count++;
						}
						else 
						{
							cluster[fg_i][fg_count] = cur_pixel;
							fg_points[fg_count][0] = i;
							fg_points[fg_count][1] = j;
							fg_count++;
						}
					}
				}


				//recalculate centroids
				double sum = 0;
				#pragma omp parallel for reduction(+:sum) num_threads(THREADS) schedule(SCHEDULE)
				for(i = 0; i < bg_count; i++)
				{
					int cur = cluster[bg_i][i];
					sum += cur;
				}
				bg_centroid = sum/bg_count;

				sum = 0;
				#pragma omp parallel for reduction(+:sum) num_threads(THREADS) schedule(SCHEDULE)
				for(i = 0; i < fg_count; i++)
				{
					int cur = cluster[fg_i][i];
					sum += cur;
				}
				fg_centroid = sum/fg_count;

				distance_unk = alpha* abs(bg_centroid - fg_centroid);

				// Check for convergence
				if(round(bg_centroid) == round(bg_centroid_prev) && 
				   round(fg_centroid) == round(fg_centroid_prev)) {
					break;
				}

				bg_centroid_prev = bg_centroid;
				fg_centroid_prev = fg_centroid;

				num_iters++;
			}

			printf("static scheduling\n");
			printf("num_threads = %d\n", THREADS); 
			printf("Convergence at num_iters = %d\n", num_iters);  

			//UNKNOWN
			int *region = (int*) malloc (sizeof(int*)*25);

			int B_max = cluster[bg_i][0]; 
			int F_max = cluster[fg_i][0];
			int B_min = cluster[bg_i][0]; 
			int F_min = cluster[fg_i][0];

			#pragma omp parallel for simd num_threads(THREADS) schedule(SCHEDULE)
			for(i = 0; i< bg_count; i++)
			{
				if(cluster[bg_i][i] < B_max) {
					B_max = cluster[bg_i][i];
				}

				if(cluster[bg_i][i] > B_min) {
					B_min = cluster[bg_i][i];
				}
			}

			#pragma omp parallel for simd num_threads(THREADS) schedule(SCHEDULE)
			for(i =0; i<fg_count; i++)
			{
				if(cluster[fg_i][i] < F_max) {
					F_max = cluster[fg_i][i];
				}

				if(cluster[fg_i][i] > F_min) {
					F_min = cluster[fg_i][i];
				}
			}

			#pragma omp parallel for simd num_threads(THREADS) schedule(SCHEDULE)
			//Unknown bucket allocation
			for(i = 0; i < unk_count ; i++)
			{
				int cur_unk_pixel = cluster[unk_i][i];
				int x = unk_points[i][0];
				int y = unk_points[i][1];

				/*for(j = 0; j < 5; j++)
				{
					cout << j << endl;;
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
				}*/


				//Bn and Fn calculations
				int Bn, Fn;
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
			totalTime += time;     

			int **output = (int**) malloc (sizeof(int*)*height);
			#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
			for(i = 0 ;i< height; i++)
			{
				output[i] = (int*) malloc (sizeof(int)*width);
			}
			
			#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
			for(i = 0; i<fg_count; i++)
			{
				int x = fg_points[i][0];
				int y = fg_points[i][1];
				output[x][y] = 1;
			}

			#pragma omp parallel for num_threads(THREADS) schedule(SCHEDULE)
			for(i = 0; i<bg_count; i++)
			{
				int x = bg_points[i][0];
				int y = bg_points[i][1];
				output[x][y] = 0;
			}

			unsigned char black = 0;
			#pragma omp parallel for collapse(2) num_threads(THREADS) schedule(SCHEDULE)
			for(i=0; i<height; i++)
			{
				for(j=0; j<width; j++) {
					if (!output[i][j]) {
						img1(j,i,0,0) = black;
						img1(j,i,0,1) = black;
						img1(j,i,0,2) = black;
					}
				}
			}


			//Display here if you want


			//Deallocation to support opening multiple images
			delete region;

			for(i = 0 ;i< height; i++)
			{
				delete output[i];
			}
			delete output;

			for (i = 0; i < 3; ++i) {
				delete cluster[i];
			}
			delete cluster;

			for (i = 0; i < height * width; ++i) {
				delete unk_points[i];
			}
			delete unk_points;

			for (i = 0; i < height * width; ++i) {
				delete fg_points[i];
			}
			delete fg_points;

			for (i = 0; i < height * width; ++i) {
				delete bg_points[i];
			}
			delete bg_points;


			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					delete image[i][j];
				}
				delete image[i];
			}
		}
	}
	printf("Execution total time = %f sec\n", totalTime);  

	//END
	return 0;
}
