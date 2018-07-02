/**
@By: Brian Wijeratne 
*/

#include "mex.h"
#include "matrix.h"
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <math.h>

#define PI 3.14159265358979323846

void point_tfrm(int width_img, int height_img, double ***input_m, double ***output_m);
int cmpfunc(const void *x, const void *y);


// PRINT TO MATLAB: To print int value, set type to 0 
void printf_matlab(char *text, double value, int type) {
	char str_value[20], str[50];

	if (type == 1)
		gcvt(value, 20, str_value);
	else
		gcvt((int)value, 20, str_value);
	strcpy(str, text); strcat(str, str_value); strcat(str, "\n");
	mexPrintf(str);
}

/**
*
*	MEX FUNCTION
*
*/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////

	if (nlhs != 1)
		mexErrMsgTxt("invert_colours can only accept one output argument(s)");

	if (nrhs != 1)
		mexErrMsgTxt("required 1 input argument(s)");

	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////

	const mxArray *img_array = prhs[0];

	//inputted img must be type double
	if (!mxIsDouble(img_array))
		mexErrMsgTxt("Images needs to be type 'double'");	

	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////


	//get size of img
	mwSize ndims = mxGetNumberOfDimensions(img_array);
	const mwSize *dims_img = mxGetDimensions(img_array);

	int height_img = dims_img[0];
	int width_img = dims_img[1];

	printf_matlab("\nImg dim: ", dims_img[2], 0);

	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////

	mxClassID input_type = mxGetClassID(img_array);


	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	/////////////// Allocate the memory for 2D array ///////////////////////

	// INPUT
	double*** input_m = (double ***)mxCalloc(width_img, sizeof(double**));			//1D - size (width_img)

	for (int x = 0; x < width_img; x++) {
		input_m[x] = (double **)mxCalloc(height_img, sizeof(double*));				//2D - size (width_img x height_img)
	}

	for (int x = 0; x < width_img; x++) {
		for (int y = 0; y < height_img; y++) {
			input_m[x][y] = (double *)mxCalloc(3, sizeof(double));					//3D - size (width_img x height_img x 3)
		}
	}

	//OUTPUT
	double*** output_m = (double ***)mxCalloc(width_img, sizeof(double**));

	for (int x = 0; x < width_img; x++) {
		output_m[x] = (double **)mxCalloc(height_img, sizeof(double*));
	}

	for (int x = 0; x < width_img; x++) {
		for (int y = 0; y < height_img; y++) {
			output_m[x][y] = (double *)mxCalloc(3, sizeof(double));
		}
	}


	////////////////////////////////////////////////////////////////////////


	mxArray *output = mxCreateNumericArray(ndims, dims_img, input_type, mxREAL);

	//Output to MATLAB
	plhs[0] = output;

	//create pointers for all imported values in order to perform calculations

	double *img_ptr = (double *)mxGetData(img_array);
	double *out_ptr = (double *)mxGetData(output);

	int img_size = height_img*width_img;

	//Copy Input img into temp_ img creating a padded img
	for (int z = 0; z < 3; z++) {
		for (int y = 0; y < height_img; y++) {
			for (int x = 0; x < width_img; x++) {
				input_m[x][y][z] = img_ptr[x*height_img + y + z*img_size];					//setup input
			}
		}
	}


	//run operation on image
	point_tfrm(width_img, height_img, input_m, output_m);


	//Copy Input img into temp_ img creating a padded img
	for (int z = 0; z < 3; z++) {
		for (int y = 0; y < height_img; y++) {
			for (int x = 0; x < width_img; x++) {
				out_ptr[x*height_img + y + z*img_size] = output_m[x][y][z];					//setup output
			}
		}
	}

}


void point_tfrm(int width_img, int height_img, double ***input_m, double ***output_m) {

	int r = 0, img_size = width_img*height_img;

	struct histogram_equilization_table
	{
		int count = 0;			//COUNT
		double p_r = 0.0;		//Pr(rk)
		double CDF = 0.0;		//CDF
		double s_k = 0;
	} histo_equ[256][3];


	//Find number of pixels at each intensity, runs through each img color level once.
	for (int y = 0; y < height_img; y++) {
		for (int x = 0; x < width_img; x++) {

			r = round(255 * input_m[x][y][0]);		//image intensity
			histo_equ[r][0].count++;				//count++ for each value of image intensity
			r = round(255 * input_m[x][y][1]);		//image intensity
			histo_equ[r][1].count++;				//count++ for each value of image intensity
			r = round(255 * input_m[x][y][2]);		//image intensity
			histo_equ[r][2].count++;				//count++ for each value of image intensity
		}
	}

	//Fill histogram equilization table
	for (int y = 0; y <= 255; y++) {

		if (histo_equ[y][0].count > 0) {
			histo_equ[y][0].p_r = ((double)histo_equ[y][0].count) / ((double)img_size);
			histo_equ[y][0].count = 0;
		}
		if (histo_equ[y][1].count > 0) {
			histo_equ[y][1].p_r = ((double)histo_equ[y][1].count) / ((double)img_size);
			histo_equ[y][1].count = 0;
		}
		if (histo_equ[y][2].count > 0) {
			histo_equ[y][2].p_r = ((double)histo_equ[y][2].count) / ((double)img_size);
			histo_equ[y][2].count = 0;
		}


		histo_equ[y][0].CDF = histo_equ[y][0].p_r;
		histo_equ[y][1].CDF = histo_equ[y][1].p_r;
		histo_equ[y][2].CDF = histo_equ[y][2].p_r;

		if (y != 0) {
			histo_equ[y][0].CDF += histo_equ[y - 1][0].CDF;
			histo_equ[y][1].CDF += histo_equ[y - 1][1].CDF;
			histo_equ[y][2].CDF += histo_equ[y - 1][2].CDF;
		}

		histo_equ[y][0].s_k = histo_equ[y][0].CDF;
		histo_equ[y][1].s_k = histo_equ[y][1].CDF;
		histo_equ[y][2].s_k = histo_equ[y][2].CDF;
	}

	//apply histogram equilization to img, runs through img once.
	for (int y = 0; y < height_img; y++) {
		for (int x = 0; x < width_img; x++) {

			r = round(255 * input_m[x][y][0]);			//image intensity
			output_m[x][y][0] = histo_equ[r][0].s_k;	//apply histogram equilization
			r = round(255 * input_m[x][y][1]);			//image intensity
			output_m[x][y][1] = histo_equ[r][1].s_k;	//apply histogram equilization
			r = round(255 * input_m[x][y][2]);			//image intensity
			output_m[x][y][2] = histo_equ[r][2].s_k;	//apply histogram equilization

		}
	}

	//ZERO each TABLE.
	for (int y = 0; y <= 255; y++) {
		histo_equ[y][0].count = 0;
		histo_equ[y][0].p_r = 0.0;
		histo_equ[y][0].CDF = 0.0;
		histo_equ[y][0].s_k = 0.0;

		histo_equ[y][1].count = 0;
		histo_equ[y][1].p_r = 0.0;
		histo_equ[y][1].CDF = 0.0;
		histo_equ[y][1].s_k = 0.0;

		histo_equ[y][2].count = 0;
		histo_equ[y][2].p_r = 0.0;
		histo_equ[y][2].CDF = 0.0;
		histo_equ[y][2].s_k = 0.0;
	}

}

int cmpfunc(const void *x, const void *y) {
	double xx = *(double*)x, yy = *(double*)y;
	if (xx < yy) return -1;
	if (xx > yy) return  1;
	return 0;
}
