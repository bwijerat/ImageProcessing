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

void point_tfrm(int width_img, int height_img, int width_h, int height_h, double **h_m, double ***input_m, double ***output_m);
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

	if (nrhs != 2)
		mexErrMsgTxt("required 2 input argument(s)");

	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////

	const mxArray *img_array = prhs[0];
	const mxArray *h_array = prhs[1];

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

	//Able to use scalar input values as pointers in my code 
	const mwSize *dims_h = mxGetDimensions(h_array);

	int height_h = dims_h[0];
	int width_h = dims_h[1];

	printf_matlab("H ", height_h, 0);
	printf_matlab("W ", width_h, 0);

	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////

	mxClassID input_type = mxGetClassID(img_array);


	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	/////////////// Allocate the memory for 3D array ///////////////////////

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

	//FILTER
	double** h_m = (double **)mxCalloc(width_h, sizeof(double*));

	for (int x = 0; x < width_h; x++) {
		h_m[x] = (double *)mxCalloc(height_h, sizeof(double));
	}


	////////////////////////////////////////////////////////////////////////


	mxArray *output = mxCreateNumericArray(ndims, dims_img, input_type, mxREAL);

	//Output to MATLAB
	plhs[0] = output;

	//create pointers for all imported values in order to perform calculations

	double *img_ptr = (double *)mxGetData(img_array);
	double *h_ptr = (double *)mxGetData(h_array);
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

	//setup filter
	for (int y = 0; y < height_h; y++) {
		for (int x = 0; x < width_h; x++) {
			h_m[x][y] = h_ptr[x*height_h + y];
		}
	}

	//run operation on image
	point_tfrm(width_img, height_img, width_h, height_h, h_m, input_m, output_m);


	//Copy Input img into temp_ img creating a padded img
	for (int z = 0; z < 3; z++) {
		for (int y = 0; y < height_img; y++) {
			for (int x = 0; x < width_img; x++) {
				out_ptr[x*height_img + y + z*img_size] = output_m[x][y][z];					//setup output
			}
		}
	}

}

// Color Image(3D point transform)
void point_tfrm(int width_img, int height_img, int width_h, int height_h, double **h_m, double ***input_m, double ***output_m) {


	int padding_y = (int)floor(height_h / 2);							//y padding
	int padding_x = (int)floor(width_h / 2);							//x padding

	int x_h, y_h, x_i, y_i, count;
	double pixel[3], MAX;													//initialized 0.0 in code

												
	for (int y = 0; y < height_img; y++) {
		for (int x = 0; x < width_img; x++) {

			pixel[0] = 0.0; pixel[1] = 0.0; pixel[2] = 0.0;

			for (int b = -padding_y; b <= padding_y; b++) {
				for (int a = -padding_x; a <= padding_x; a++) {

					//img indexes
					x_i = x + a;
					y_i = y + b;

					//mirroring
					if (x_i < 0)
						x_i = abs(x_i) - 1;
					else if (x_i >= width_img)
						x_i = (width_img - 1) - (x_i - width_img);

					if (y_i < 0)
						y_i = abs(y_i) - 1;
					else if (y_i >= height_img)
						y_i = (height_img - 1) - (y_i - height_img);

					x_h = a + padding_x;
					y_h = b + padding_y;

					pixel[0] += h_m[x_h][y_h] * input_m[x_i][y_i][0];
					pixel[1] += h_m[x_h][y_h] * input_m[x_i][y_i][1];
					pixel[2] += h_m[x_h][y_h] * input_m[x_i][y_i][2];

				}
			}

			output_m[x][y][0] = pixel[0];
			output_m[x][y][1] = pixel[1];
			output_m[x][y][2] = pixel[2];
		}
	}

}

int cmpfunc(const void *x, const void *y) {
	double xx = *(double*)x, yy = *(double*)y;
	if (xx < yy) return -1;
	if (xx > yy) return  1;
	return 0;
}
