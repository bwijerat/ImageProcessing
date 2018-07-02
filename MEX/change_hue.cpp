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

void point_tfrm(int width_img, int height_img, double *angle, double ***input_m, double ***output_m);
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
	const mxArray *A_array = prhs[1];

	//inputted img must be type double
	if (!mxIsDouble(img_array))
		mexErrMsgTxt("Images needs to be type 'double'");

	//scalar must be taken as doubles
	double *angle = (double*)mxGetData(A_array);

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


	///////////////////////////////////////////////////////////////////////
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
	point_tfrm(width_img, height_img, angle, input_m, output_m);


	//Copy Input img into temp_ img creating a padded img
	for (int z = 0; z < 3; z++) {
		for (int y = 0; y < height_img; y++) {
			for (int x = 0; x < width_img; x++) {
				out_ptr[x*height_img + y + z*img_size] = output_m[x][y][z];					//setup output
			}
		}
	}

}


void point_tfrm(int width_img, int height_img, double *angle, double ***input_m, double ***output_m) {

	double r, g, b, H, S, I, O, den, num;
	double rgb[3];						// red - 0, green - 1, blue - 2

										//Copy Input img into temp_ img creating a padded img
	for (int y = 0; y < height_img; y++) {
		for (int x = 0; x < width_img; x++) {


			//******* FROM RGB TO HSI *******//

			//RED						//GREEN						//BLUE
			rgb[0] = input_m[x][y][0];  rgb[1] = input_m[x][y][1];  rgb[2] = input_m[x][y][2];
			r = input_m[x][y][0];		g = input_m[x][y][1];		b = input_m[x][y][2];

			num = 0.5*((r - g) + (r - b));
			den = sqrt(pow((r - g), 2) + (r - b)*(g - b));

			O = acos(((double)num / (double)(den + 0.000001)));

			if (b > g) {
				O = 2.0*PI - O;
			}

			H = O + (*angle*PI);
			qsort(rgb, sizeof(rgb) / sizeof(double), sizeof(double), cmpfunc);
			S = (1 - (3 / (r + g + b + 0.000001))*rgb[0]);
			I = (double)(r + g + b) / 3.0;
			

			//******* FROM HSI TO RGB *******//

			if (0.0 <= H < ((2.0 / 3.0)*PI)) {
				b = I*(1.0 - S);
				r = I*(1.0 + ((double)S*cos(H) / (cos((PI / 3.0) - H) + 0.000001)));
				g = 3.0*I - (r + b);
			}
			else if (((2.0 / 3.0)*PI) <= H < ((4.0 / 3.0)*PI)) {
				H = H - (2.0 / 3.0)*PI;
				r = I*(1.0 - S);
				g = I*(1.0 + ((double)S*cos(H) / (cos((PI / 3.0) - H) + 0.000001)));
				b = 3.0*I - (r + g);
			}
			else if (((4.0 / 3.0)*PI) <= H <= (2.0*PI)) {
				H = H - (4.0 / 3.0)*PI;
				g = I*(1.0 - S);
				b = I*(1.0 + ((double)S*cos(H) / (cos((PI / 3.0) - H) + 0.000001)));
				r = 3.0*I - (g + b);
			}
			else {
				printf_matlab("\nERROR: H < 0 OR H > 2pi", NULL, 0);
			}

			output_m[x][y][0] = abs(r);
			output_m[x][y][1] = abs(g);
			output_m[x][y][2] = abs(b);
		}
	}

}

int cmpfunc(const void *x, const void *y) {
	double xx = *(double*)x, yy = *(double*)y;
	if (xx < yy) return -1;
	if (xx > yy) return  1;
	return 0;
}
