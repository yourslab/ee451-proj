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
#include <pthread.h>

using namespace std;
using namespace cimg_library;

#define input_file  "input.raw"
#define output_file "output.txt"

#define THREADS 2

struct thread_data {
	int tid;
	int widthStart;
	int widthEnd;
	int heightStart;
	int heightEnd;
	int ***image;
	int **output;
	int background;
	int bg_centroid;
	double fg_centroid;
	double distance_unk;
	int ***cluster;
	int ***unk_points;
	int ***fg_points; 
	int ***bg_points;
	int *bgClusterSize;
	int *fgClusterSize;
	int *unkClusterSize;
	int *bgTotal;
	int *fgTotal;
};

struct  thread_data* thread_data_array;

void *AssignToCluster(void *threadarg) {
	struct thread_data* my_data;
	my_data = (struct thread_data *) threadarg;

	int tid = my_data->tid;

	int widthStart = my_data->widthStart;
	int widthEnd = my_data->widthEnd;
	int heightStart = my_data->heightStart;
	int heightEnd = my_data->heightEnd;

	int ***image = my_data->image;
	int **output = my_data->output;

	int background = my_data->background;

	int bg_centroid = my_data->bg_centroid;
	double fg_centroid = my_data->fg_centroid;
	double distance_unk = my_data->distance_unk;

	int ***cluster = my_data->cluster;
	int ***unk_points = my_data->unk_points;
	int ***fg_points = my_data->fg_points; 
	int ***bg_points = my_data->bg_points;

	int *bgClusterSize = my_data->bgClusterSize;
	int *fgClusterSize = my_data->fgClusterSize;
	int *unkClusterSize = my_data->unkClusterSize;
	int *bgTotal = my_data->bgTotal;
	int *fgTotal = my_data->fgTotal;

	bgClusterSize[tid] = 0;
	fgClusterSize[tid] = 0;
	unkClusterSize[tid] = 0;

	int dev = 40, X_CONSTANT =128;
	double alpha = 0.5; //NEED TO SET
	int bg_i = 0, fg_i =1, unk_i = 2;

	int i,j,k;

	//initial bucket allocation 
	for(i = heightStart; i < heightEnd; i++)
	{
		for(j = widthStart; j < widthEnd; j++)
		{
			int cur_pixel = image[i][j][background];
			int distance_bg = pow(cur_pixel - bg_centroid, 2.0);
			int distance_fg = pow(cur_pixel - fg_centroid, 2.0);

			if(abs(distance_bg - distance_fg) < distance_unk) {
				int unk_count = unkClusterSize[tid];
				cluster[tid][unk_i][unk_count] = cur_pixel;
				unk_points[tid][unk_count][0] = i;
				unk_points[tid][unk_count][1] = j;
				unkClusterSize[tid]++;
			}
			else if(distance_bg < distance_fg)
			{
				int bg_count = bgClusterSize[tid];
				cluster[tid][bg_i][bg_count] = cur_pixel;
				bg_points[tid][bg_count][0] = i;
				bg_points[tid][bg_count][1] = j;
				bgClusterSize[tid]++;
			}
			else 
			{
				int fg_count = fgClusterSize[tid];
				cluster[tid][fg_i][fg_count] = cur_pixel;
				fg_points[tid][fg_count][0] = i;
				fg_points[tid][fg_count][1] = j;
				fgClusterSize[tid]++;
			}
		}
	}

	//recalculate centroids
	double sum = 0;
	for(i = 0; i < bgClusterSize[tid]; i++)
	{
		int cur = cluster[tid][bg_i][i];
		sum += cur;
	}
	bgTotal[tid] = sum/bgClusterSize[tid];

	sum = 0;
	for(i = 0; i < fgClusterSize[tid]; i++)
	{
		int cur = cluster[tid][fg_i][i];
		sum += cur;
	}
	fgTotal[tid] = sum/fgClusterSize[tid];

}

//Returns 3D Array of YCbCr Values for each pixel with [height][width][YCbCr (Y - 0, Cb - 1, Cr -2]
int*** getYCbCr(CImg<unsigned char> img1) {
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

	ofstream myfile;
	myfile.open ("example.txt");

	CImg<unsigned char> img2;
	img2 = img1.get_RGBtoYCbCr();

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
	const string inputObjectName[] = {"flipphones", "forks", "hammers", "mugs", "pliers", "scissors"};
	string filename;
	CImg<unsigned char> img1;
	int i,j,k; //loop variables

	double totalTime = 0.0f;
	for (int objectIndex = 0; objectIndex < 6; objectIndex++) {
		for (int imageIndex = 0; imageIndex < 9; imageIndex++) {
			pthread_t threads[THREADS];
			int *thread_ids[THREADS];
			int rc;
			thread_data_array = (struct thread_data*) malloc(sizeof(struct thread_data) * THREADS);

			std::ostringstream ss;
			ss << "images/" << inputObjectName[objectIndex] << "." << setfill('0') << setw(3) << imageIndex << ".jpg";
			filename = ss.str();
			cout << "Filename: " << filename << endl;

			img1.load_jpeg(filename.c_str());
			int width = img1.width();
			int height = img1.height();
			
			
			int **output = new int*[height];
			for(i = 0 ;i< height; i++)
			{
				output[i] = new int[width];
			}

			int ***cluster = new int**[THREADS];
			for (i = 0; i < THREADS; ++i) {
				cluster[i] = new int*[3];
				for (j = 0; j < 3; j++) {
					cluster[i][j] = new int[height*width/THREADS];
				}
			}

			int ***unk_points = new int**[THREADS];
			for (i = 0; i < THREADS; ++i) {
				unk_points[i] = new int*[height*width/THREADS];
				for (j = 0; j < height*width/THREADS; j++) {
					unk_points[i][j] = new int[2];
				}
			}

			int ***fg_points = new int**[THREADS];
			for (i = 0; i < THREADS; ++i) {
				fg_points[i] = new int*[height*width/THREADS];
				for (j = 0; j < height*width/THREADS; j++) {
					fg_points[i][j] = new int[2];
				}
			}

			int ***bg_points = new int**[THREADS];
			for (i = 0; i < THREADS; ++i) {
				bg_points[i] = new int*[height*width/THREADS];
				for (j = 0; j < height*width/THREADS; j++) {
					bg_points[i][j] = new int[2];
				}
			}

			int *bgClusterSize = new int[THREADS];
			int *fgClusterSize = new int[THREADS];
			int *unkClusterSize = new int[THREADS];

			int *bgTotal = new int[THREADS];
			int *fgTotal = new int[THREADS];
			
			
			int dev = 40, X_CONSTANT =128;
			int Cr = 2, Cb = 1, Y=0;
			int background = Cr;
			int bg_i = 0, fg_i =1, unk_i = 2;
			double alpha = 0.5; //NEED TO SET

			// measure the start time here
			struct timespec start, stop; 
			double time;
			int ***image = getYCbCr(img1);

			if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}


			if(image[0][0][Cb] >= 128)
			{
				background = Cb;
			}
			
			double bg_centroid = image[0][0][background];
			double fg_centroid = bg_centroid + dev;
			double distance_unk = alpha*abs(bg_centroid - fg_centroid);

			double bg_centroid_prev = 0;
			double fg_centroid_prev = 0;

			int num_iters = 1;
			for(int k = 0; k < num_iters; k++)
			{
				//initial bucket allocation 
				for (i = 0; i < THREADS; ++i) {
					thread_data_array[i].tid = i;
					thread_data_array[i].widthStart = 0;
					thread_data_array[i].widthEnd = width;
					thread_data_array[i].heightStart = height/THREADS * i;
					thread_data_array[i].heightEnd = height/THREADS * (i+1);
					thread_data_array[i].image = image;
					thread_data_array[i].output = output;
					thread_data_array[i].background = background;
					thread_data_array[i].bg_centroid = bg_centroid;
					thread_data_array[i].fg_centroid = fg_centroid;
					thread_data_array[i].distance_unk = distance_unk;
					thread_data_array[i].cluster = cluster;
					thread_data_array[i].unk_points = unk_points;
					thread_data_array[i].fg_points = fg_points; 
					thread_data_array[i].bg_points = bg_points;
					thread_data_array[i].bgClusterSize = bgClusterSize;
					thread_data_array[i].fgClusterSize = fgClusterSize;
					thread_data_array[i].unkClusterSize = unkClusterSize;
					thread_data_array[i].bgTotal = bgTotal;
					thread_data_array[i].fgTotal = fgTotal;
					
					rc = pthread_create(&threads[i], NULL, AssignToCluster, (void *) &thread_data_array[i] );
					if (rc) { printf("ERROR; return code from pthread_create() is %d\n", rc); exit(-1);}
				}

				for (i = 0; i < THREADS; ++i) {
					rc = pthread_join(threads[i], NULL);
					if (rc) { printf("ERROR; return code from pthread_join() is %d\n", rc); exit(-1);}
				}

				//recalculate centroids
				double sum = 0;
				int bg_count = 0;
				for(i = 0; i < THREADS; i++)
				{
					for (j = 0; j < bgClusterSize[i]; j++) {
						int cur = cluster[i][bg_i][j];
						sum += cur;
					}
					bg_count += bgClusterSize[i];
				}
				bg_centroid = sum/bg_count;

				sum = 0;
				int fg_count = 0;
				for(i = 0; i < THREADS; i++)
				{
					for (j = 0; j < fgClusterSize[i]; j++) {
						int cur = cluster[i][fg_i][j];
						sum += cur;
					}
					fg_count += fgClusterSize[i];
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

			printf("Convergence at num_iters = %d\n", num_iters);  

			/*int B_max = 255; 
			int F_max = 255;
			int B_min = 0; 
			int F_min = 0;

			for(i = 0; i< bg_count; i++)
			{
				if(cluster[bg_i][i] < B_max) {
					B_max = cluster[bg_i][i];
				}

				if(cluster[bg_i][i] > B_min) {
					B_min = cluster[bg_i][i];
				}
			}

			for(i =0; i<fg_count; i++)
			{
				if(cluster[fg_i][i] < F_max) {
					F_max = cluster[fg_i][i];
				}

				if(cluster[fg_i][i] > F_min) {
					F_min = cluster[fg_i][i];
				}
			}

			//Unknown bucket allocation
			for(i = 0; i < unk_count ; i++)
			{
				int cur_unk_pixel = cluster[unk_i][i];
				int x = unk_points[i][0];
				int y = unk_points[i][1];

				// for(j = 0; j < 5; j++)
				// {
				// 	cout << j << endl;;
				// 	for( k = 0; k < 5; k++)
				// 	{
				// 		if(x-2+j < 0 && y-2+k <0) region[j+k] = image[0][0][background];
				// 		else if(x-2+j >= height && y-2+k <0 ) region[j+k] = image[height-1][0][background];
				// 		else if(x-2+j < 0 && y-2+k >= width) region[j+k] = image[0][width -1][background];      
				// 		else if(x-2+j < 0) region[j+k] = image[0][y-2+k][background];
				// 		else if(y-2+k < 0) region[j+k] = image[x-2+j][0][background];
				// 		else if(x-2+j >= height && y-2+k >= width) region[j+k] = image[height-1][width -1][background];
				// 		else if(x-2+j >= height) region[j+k] = image[height-1][y-2+k][background];
				// 		else if(y-2+k >= width) region[j+k] = image[x-2+j][width - 1][background];
				// 		else region[j+k] = image[x-2+j][y-2+j][background];
				// 	}
				// }


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
			}*/

			if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) { perror("clock gettime");}       
				time = (stop.tv_sec - start.tv_sec)+ (double)(stop.tv_nsec - start.tv_nsec)/1e9;

				// print out the execution time here
			printf("Execution time = %f sec\n", time);      
			
			totalTime += time;

/*			unsigned char black = 0;
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
*/
			//Display here if you want

			free(thread_data_array);

			delete bgClusterSize;
			delete fgClusterSize;
			delete unkClusterSize;

			delete bgTotal;
			delete fgTotal;

			//Deallocation to support opening multiple images
			for(i = 0 ;i< height; i++)
			{
				delete[] output[i];
			}
			delete output;

			for (i = 0; i < THREADS; ++i) {
				for (j = 0; j < 3; j++) {
					delete[] cluster[i][j];
				}
				delete[] cluster[i];
			}
			delete cluster;

			for (i = 0; i < THREADS; ++i) {
				for (j = 0; j < height*width/THREADS; j++) {
					delete[] unk_points[i][j];
				}
				delete[] unk_points[i];
			}
			delete unk_points;

			for (i = 0; i < THREADS; ++i) {
				for (j = 0; j < height*width/THREADS; j++) {
					delete[] fg_points[i][j];
				}
				delete[] fg_points[i];
			}
			delete fg_points;

			for (i = 0; i < THREADS; ++i) {
				for (j = 0; j < height*width/THREADS; j++) {
					delete[] bg_points[i][j];
				}
				delete[] bg_points[i];
			}
			delete bg_points;


			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					delete[] image[i][j];
				}
				delete[] image[i];
			}
			delete image;

		}
	}
	printf("Execution total time = %f sec\n", totalTime);      

	//END
	return 0;
}
