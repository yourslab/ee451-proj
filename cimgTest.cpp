#include "CImg.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;
using namespace cimg_library;

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

int main() {
	int ***yCbCrArray = getYCbCr("pink.jpg");
	cout << yCbCrArray[0][5][2] << endl;
	return 0;
}