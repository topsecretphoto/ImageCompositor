/*
	***** Dan Hale - CPSC 6040 - Clemson University - 12.2.17 *****

	Welcome to "Dan's Star Wars Compositer ...awesome it is...!" *working title

	The purpose of this program is to re-create the open scene from Star Wars Episode 4 - A New Hope.

	One may either use the provided images or select their own to create a unique animation

	***** 

	To compile:

	make final

	To run the program at the command line:

	./final 


	*****

	Background image must be 1280 pixels x 1440 pixels.

	Ship images must be 1280 pixels x 720 pixels.

	If you wish to write out the image files, give the desired file name base when prompted.

*/

#include "matrix.h"

#include <OpenImageIO/imageio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cmath>  
#include <cstdio>
#include <cstdlib>
#include <string>
#include <math.h> 
#include <stdlib.h>

#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#define maximum(x, y, z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
#define minimum(x, y, z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))

using namespace std;
using std::string;
OIIO_NAMESPACE_USING

int xres; // image width
int yres; // image height
int channels; // number of channels and image has initially
unsigned char* pixels; // input vector before 4 channel conversion
unsigned char* fourChan; // holds original RGBA pixel data
unsigned char* newdisplay; // holds convolved RGBA pixel data

float* data; // holds image pixel data before transformation
float ** pixmap; // holds row pointers for pixmap
float* newData; // holds image pixel data after transformation
float ** newMap; // holds row pointers for newData
float* displayData; // holds image pixel data that is being displayed
float ** displayMap; // holds row pointers for displayData
float* backData; // holds image RGBA pixel data for background image
float ** backMap; // holds row pointers for backData
float* imgOneData; // holds original pixel data for image one
float ** imgOneMap; // holds row pointers for imgOneData
float* imgTwoData; // holds original pixel data for image two
float ** imgTwoMap; // holds row pointers for imgTwoData

string infilename; // title screen file name
string outfilename; // the full output filename
string userName; // the user defined output filename base

bool outSwitch = false; // controls whether or not images are written out
int picSwap = 1; // switches between ship one and ship two in pixswap()
int yMax; // image height after transform
int xMax; // image width after transform
int xAdjust; // displacement if xMin is negative
int yAdjust; // displacement if yMin is negative
int pan; // controls pan rate

double hueDrop = 120.0; // determines masking color (green screen, blue screen, etc
double hueRange = 30.0; // contols hue variation elimination
double feather = (hueRange / 10.0); // controls feathering for alpha edges

int displaySize = (1280*720*4); // fixed size of display vector
int lineSize = (1280*4); // fixed size of display scanline

int imageCount = 0; // numbers images written out

// initialize transformation matrix to identity
Matrix3D M;


/*
		Calls the glDrawPixels function to trasfer the data in the display vector
		to the screen. If not file information has been provided, we just draw 
		a black square and await further instruction.
*/
void displayImage(){

  // specify window clear (background) color to be black
  glClearColor(0, 0, 0, 0);
  
  glClear(GL_COLOR_BUFFER_BIT);  // clear window to background color
	
	glDrawPixels(1280, 720, GL_RGBA, GL_UNSIGNED_BYTE, newdisplay);

  glFlush();  // flush the OpenGL pipeline to the viewport
}

/*
	sends information directly to the display for opening screen
*/
void toDisplay(){

		for (int i = 0; i < (xres*yres*4); i++) {	
			newdisplay[i] = fourChan[i];
		}
}


/*
	This is the function copies the green screen image with user defined alpha mask
	into a seperate vector for output before the image is combined with a white 
	background for display.
*/
void setPixmap() {

	float value;
	float dispVal;

	pixmap = new float*[yres];
	data = new float[xres*yres*4];

	for (int i = 0; i < (xres*yres*4); i++) {	
		dispVal = fourChan[i];
		data[i] = dispVal;
	}
	
	pixmap[0] = data;

	for (int i = 1; i < (yres); i++) {
		pixmap[i] = pixmap[i-1] + (xres * 4);
	}

}


/*
	swaps information from ship one to ship two (and back) before transform performed
*/
void swapPixmap() {

	float value;
	float dispVal;

	if (picSwap == 0) {
		for (int i = 0; i < (xres*yres*4); i++) {	
			dispVal = imgOneData[i];
			data[i] = dispVal;
		}
	}

	if (picSwap == 1) {
		for (int i = 0; i < (xres*yres*4); i++) {	
			dispVal = imgTwoData[i];
			data[i] = dispVal;
		}
	}

	picSwap = abs(picSwap - 1);

	pixmap[0] = data;

	for (int i = 1; i < (yres); i++) {
		pixmap[i] = pixmap[i-1] + (xres * 4);
	}

}


/*
	This is the function copies the green screen image with user defined alpha mask
	into a seperate vector for output before the image is combined with a white 
	background for display.
*/
void setDisplayMap() {

	float value;
	float dispVal;

	displayMap = new float*[yres];
	displayData = new float[xres*yres*4]; 

	for (int i = 0; i < (xres*yres*4); i++) {	
		displayData[i] = static_cast<float>(newdisplay[i]);
	}

	displayMap[0] = displayData;

	for (int i = 1; i < (yres); i++) {
		displayMap[i] = displayMap[i-1] + (lineSize);
	}

}


/*
	stores display information back into a vector so second image can be comp'ed in
*/
void updateDisplayMap() {

	for (int i = 0; i < (xres*yres*4); i++) {	
		displayData[i] = static_cast<float>(newdisplay[i]);
	}

}


/*
	Copies information from the display vector to the background vector
*/
void setBack() {

	float dispVal;
	backMap = new float*[yres];
	backData = new float[xres*yres*4]; 

	for (int i = 0; i < (xres*yres*4); i++) {	
		dispVal = fourChan[i];
		backData[i] = dispVal;
	}
	
	backMap[0] = data;

	for (int i = 1; i < (yres); i++) {
		backMap[i] = backMap[i-1] + (xres * 4);
	}
}


/*
	contains image one original pixel data before transformations
*/
void setImgOne() {

	float dispVal;
	imgOneMap = new float*[yres];
	imgOneData = new float[xres*yres*4]; 

	for (int i = 0; i < (xres*yres*4); i++) {	
		dispVal = fourChan[i];
		imgOneData[i] = dispVal;
	}
	
	imgOneMap[0] = data;

	for (int i = 1; i < (yres); i++) {
		imgOneMap[i] = imgOneMap[i-1] + (xres * 4);
	}
}


/*
	contains image two original pixel data before transformations
*/
void setImgTwo() {

	float dispVal;
	imgTwoMap = new float*[yres];
	imgTwoData = new float[xres*yres*4]; 

	for (int i = 0; i < (xres*yres*4); i++) {	
		dispVal = fourChan[i];
		imgTwoData[i] = dispVal;
	}
	
	imgTwoMap[0] = data;

	for (int i = 1; i < (yres); i++) {
		imgTwoMap[i] = imgTwoMap[i-1] + (xres * 4);
	}
}

/*
	Copies background image information to display during pan down
*/
void backDisplay(){

		float value;

		for (int i = 0; i < displaySize; i++) {	
			value = backData[pan];
			newdisplay[i] = value;
			pan++;
		}
		glutSwapBuffers;
}


/*
		This function converts any type of input file to a 4 channel RGBA file
		and stores it in a seperate vector called fourChan.  This allows us to 
		maintain all of our original data in the pixel vector in case we ever need 
		it to revert back from a manipulated state. The display vector is what will 
		be read out to the display.
*/
void fourChanCopy() {

	int alphaCount = 0;
	int pixelValue;

	for (int i = 0; i < (xres*yres*channels); i++) {	
		
		if (channels == 1) { // converts 1 channel grayscale images to 4 channel RGBA
												 // and copies information to the fourChan vector
			pixelValue = pixels[i];
			for (int j = 0; j < 3; j++) {
				fourChan[alphaCount] = pixelValue;
				alphaCount++;
			}
			fourChan[alphaCount] = 255;
			alphaCount++;	
		}
 
		if (channels == 2) { // converts 2 channel grayscale images to 4 channel RGBA
												 // and copies information to the fourChan vector
			pixelValue = pixels[i];
			for (int j = 0; j < 3; j++) {
				fourChan[alphaCount] = pixelValue;
				alphaCount++;
			}
			i++;
			fourChan[alphaCount] = pixels[i];
			alphaCount++;	
		}
	
		if (channels > 2) { // converts 3 channel RGB images to 4 channel RGBA and
											  // and copies information to the fourChan vector for both
												// new RGBA images and images that were initially RGBA
			pixelValue = pixels[i];
			fourChan[alphaCount] = pixelValue;
			alphaCount++;

			if (channels == 3) {
				if ((alphaCount + 1) % 4 == 0) {
				fourChan[alphaCount] = 255;
				alphaCount++;	
				}
			}
		}
	}
}


/*
		Utilizes the OpenImageIO read function for individual scanlines to ensure that
		the resulting display remains right-side up.
*/
void readScanline() {

  ImageInput *in = ImageInput::open (infilename);
  if (! in)
  return;
  const ImageSpec &spec = in->spec();
  xres = spec.width;
  yres = spec.height;
  channels = spec.nchannels;
  // std::vector<unsigned char> pixels (xres*yres*channels);
	pixels = new unsigned char[xres*yres*channels]; 
	fourChan = new unsigned char[xres*yres*4];  
	int scanlinesize = spec.width * spec.nchannels * sizeof(pixels[0]);
	in->read_image (TypeDesc::UINT8,
			(char *)pixels+(yres-1)*scanlinesize, // offset to last
			AutoStride, // default x stride
			-scanlinesize, // special y stride
			AutoStride); // default z stride
  in->close ();

	fourChanCopy(); // function that will convert image file to RGBA if needed
	
}



/*
Input RGB colo r primary values : r , g, and b on scale 0 − 255
Output HSV colo rs : h on scale 0−360, s and v on scale 0−1
*/
void RGBtoHSV(int r, int g, int b, double &h, double &s, double &v){

	double red, green, blue;
	double max, min, delta;

	red = r / 255.0; green = g / 255.0; blue = b / 255.0; /* r , g, b to 0 − 1 scale */
	
	max = maximum(red, green, blue);
	min = minimum(red, green, blue);
	
	v = max; /* value i s maximum of r , g, b */

	if (max == 0) { /* saturation and hue 0 i f value i s 0 */
		s = 0;
		h = 0;
	} else {
		s = (max - min) / max; /* saturation is color purity on scale 0 − 1 */

		delta = max - min;
		if (delta == 0) { /* hue doesn ’ t matter i f saturation i s 0 */
			h = 0;
		} else {
			if (red == max) { /* otherwise , determine hue on scale 0 − 360 */
				h = (green - blue) / delta;
			} else if (green == max) {
				h = 2.0 + (blue - red) / delta;
			} else /*( blue == max)*/ {
				h = 4.0 + (red - green) / delta;
			}
			h = h * 60.0;
			if(h < 0) {
				h = h + 360.0;
			}
		}
	}
}

/*
	This function performs the task of premultiplying each of the RGB channels 
	and combines them with a white background to allow the user to see the newly
	created alpha channel's effect.
*/
void compImages() {

	int i = 0; // background iterator
	int j = 0; // top image interator

	int crop = abs((xMax - 1280) *4); // adjust scanline for larger images
	int scaleSize = (xMax * 4); // adjusts scanline for smaller images
	int iAlphaPos = 3; // marks the position of the alpha mask 
	int jAlphaPos = 3; // marks the position of the alpha mask 
	int scaleLine = 0; // calculates length of scanline for smaller top image

	double aAlpha; // top image alpha value
	double bAlpha; // bottom image alpha value

	double preMultA; // top image pixel value
	double preMultB; // bottom image pixel value

	bool scaleSwitch = false; // turns on if top image is smaller

	if (xMax < 1280) { 
		scaleSwitch = true;
	}

	while (i < (displaySize)) {	// !!!!! used to be xres*yres*4

			if (i == iAlphaPos) { // if this is the alpha channel

				// calculate combined alpha value
				newData[j] = static_cast<int>(255 * (aAlpha + (1- aAlpha) * bAlpha));

				i++; 
				j++;

				iAlphaPos = (i + 3); // advance to next alpha value
				jAlphaPos = (j + 3); // advance to next alpha value

			} else {  // if this is NOT the alpha channel

				// set variable equal to pixel channel value for top image
				preMultA = static_cast<double>(newData[j]);
				// set variable equal to corresponding alpha value for pixel
				aAlpha = static_cast<double>(newData[jAlphaPos]);

				aAlpha = (aAlpha / 255.0); // set alpha to 0-1
				preMultA = (preMultA * aAlpha); // premultiply pixel by alpha value

				// set variable equal to pixel channel value for background image
				preMultB = static_cast<double>(displayData[i]);
				// set variable equal to corresponding alpha value for pixel
				bAlpha = static_cast<double>(displayData[iAlphaPos]);

				bAlpha = (bAlpha / 255.0); // set alpha to 0-1
				preMultB = (preMultB * bAlpha); // premultiply pixel by alpha value

				// calculate combined pixel value
				preMultA = (preMultA + (1- aAlpha) * preMultB);

				// update vector with combined pixel value
				newdisplay[i] = static_cast<int>(preMultA);
			
				j++;
				i++;
				
			}
			
			scaleLine++;

			// if top image is larger, adjust vector position accordingly 
			// at the end of the display scanline
			if (i % lineSize == 0 && scaleSwitch == false) {
					j = (j + crop);
					iAlphaPos = (i + 3);
					jAlphaPos = (j + 3);
			}

			// if top image is smaller, adjust vector position accordingly 
			// at the end of the image scanline
			if (scaleLine == scaleSize && scaleSwitch == true) {
					i = (i + crop);
					iAlphaPos = (i + 3);
					jAlphaPos = (j + 3);
					scaleLine = 0;
			}
	}	
}


/*
	This is the function that creates an alpha mask for a green screen image
*/
void createMask() {

	int channelCount = 0;
	int r; // red pixel value
	int g; // green pixel value
	int b; // blue pixel value
	double h; // pixel hue 
	double s; // pixel saturation  
	double v; // pixel value 

	double bottom = (hueDrop - hueRange); // sets the low end of the alpha mask range
	double bottomF = (hueDrop - hueRange + feather); // defines the bottom feather zone
	double top = (hueDrop + hueRange); // sets the high end of the alpha mask range
	double topF = (hueDrop + hueRange - feather); // defines the top feather zone

	int fVal;

	for (int i = 0; i < (xres*yres*4); i++) {
		channelCount++; // channel iterator
		if (channelCount == 1) { 
			r = fourChan[i]; // set red channel value
		} else if (channelCount == 2) {
			g = fourChan[i]; // set green channel value
		} else if (channelCount == 3) {
			b = fourChan[i]; // set blue channel value
		} else {

			RGBtoHSV(r,g,b,h,s,v);  // call function to translate values to HSV
			
			// If hue falls between the bottom and top feather, alpha mask is set to 0.
			// We disregard hues within range but below a value of .15 because of the 
			// tendancy for dropout in shadows and reflections in pupils and we disregard 
			// anything with a saturation less than .2 to keep gray tones within the hue range
			if (((h > bottomF) && (h < topF)) && (v > .15) && (s > .2)) { 
				fourChan[i] = 0; 

			// If hue falls between the feather value and to edge of the user determined range
			// then we generate and alpha value proportional to it's distance to the edge of the 
			// range. We continue to disregard hues within range but below a value of .15 and
			// a saturation below .2
			} else if ((((bottom < h) && (h < bottomF)) || ((top > h) && (h > topF))) && (v > .15) && (s > .2)) { 
				if (h < hueDrop) {
					fVal = static_cast<int>(((h - bottom) / (bottomF - bottom)) * 255.0);
				} else if (h > hueDrop) {
					fVal = static_cast<int>(((top - h) / (top - topF)) * 255.0);
				}
				fourChan[i] = fVal;

			// hue and value are outside the set range and feather, set alpha to 255
			} else { 
				fourChan[i] = 255;
			}

			channelCount = 0; // reset channel iterator

		}		
	}
}



/*
		Utilizes the OpenImageIO write function for individual scanlines to ensure that
		the resulting output remains right-side up.
*/
void danWrite(){

	std::ostringstream oss;
	oss << "images/" << userName << imageCount << ".jpg";
	outfilename = oss.str();
	imageCount++;

  // create the oiio file handler for the image
  ImageOutput *outfile = ImageOutput::create(outfilename);

	int scanlinesize = 1280 * 4 * sizeof(newdisplay[0]);
	ImageOutput *out = ImageOutput::create (outfilename);
	if (! out)
	return;
	ImageSpec spec (1280, 720, 4, TypeDesc::UINT8);
	out->open (outfilename, spec);
	out->write_image (TypeDesc::UINT8,
				(char *)newdisplay+(720-1)*scanlinesize, // offset to last
				AutoStride, // default x stride
				-scanlinesize, // special y stride
				AutoStride); // default z stride
	out->close ();

}


/*
	Multiply M by a scale matrix 
*/
void Scale(Matrix3D &M, float sx, float sy) {
	int row, col;
	Matrix3D R; 

	R[1][1] = sy;
	R[0][0] = sx;

	Matrix3D prod = R * M;

	for (row = 0; row < 3; row++) {
		for (col = 0; col < 3; col++) {
			M[row][col] = prod[row][col];
		}
	}
}

/*
	Multiply M by a translate matrix 
*/
void Translate(Matrix3D &M, float tx, float ty) {
	int row, col;
	Matrix3D R; 

	R[1][2] = ty;
	R[0][2] = tx;

	Matrix3D prod = R * M;

	for (row = 0; row < 3; row++) {
		for (col = 0; col < 3; col++) {
			M[row][col] = prod[row][col];
		}
	}
}


/*
	Calculate the corner positions to find new maximum and minimum values needed 
	to resize the window and logs the offset needed to keep all values above 0
*/
void calcCorners(Matrix3D &M) {

	int xMin = 0;
	int yMin = 0;
	xMax = 0;
	yMax = 0;

	Vector2D Point;

	for (int i = 0; i < 4; i++) {

		if (i % 2 == 0) {
			Point.x = 0; // set x to 0
		} else {
			Point.x = xres; // set x to maximum value
		}

		if (i < 2) {
			Point.y = 0; // set x to 0
		} else {
			Point.y = yres; // set y to maximum value
		}

		Point = M * Point; // multiply vector by matrix

		if (Point.x < xMin) { // adjust min if needed
			xMin = Point.x;
		}		
		if (Point.x > xMax) { // adjust max if needed
			xMax = Point.x;
		}
		if (Point.y < yMin) { // adjust min if needed
			yMin = Point.y;
		}
		if (Point.y > yMax) { // adjust max if needed
			yMax = Point.y;
		}
	}

	// if there are negative values for x, move image over until min = 0
	// and store value
	if (xMin < 0) {
		xAdjust = 0 - xMin;
		xMin = xMin + xAdjust;
		xMax = xMax + xAdjust;
	}

	// if there are negative values for y, move image up until min = 0
	// and store value
	if (yMin < 0) {
		yAdjust = 0 - yMin;
		yMin = yMin + yAdjust;
		yMax = yMax + yAdjust;
	}

}


/*
	create a new pixmap based on updated size of transformed image 
*/
void setNewMap() {

	newMap = new float*[1440];
	newData = new float[2560*1440*4];

	for (int i = 0; i < (2560*1440*4); i++) {	
		newData[i] = 0;
	}
	
	newMap[0] = newData;

	for (int i = 1; i < (1440); i++) {
		newMap[i] = newMap[i-1] + (xMax * 4);
	}
}


/*
	clears the destination pixmap 
*/
void clearNewMap() {

	for (int i = 0; i < (2560*1440*4); i++) {	
		newData[i] = 0;
	}
	
	newMap[0] = newData;

	for (int i = 1; i < (1440); i++) {
		newMap[i] = newMap[i-1] + (xMax * 4);
	}
}


/*
	Inverse map each point in the transformed image back to it's source point
*/
void invMap(Matrix3D &M) {

	int newX; // x value for transformed image
	int newY; // y value for transformed image
	int xVal = 0; // x value for original image
	int yVal = 0; // y value for original image 

	Vector2D Point; // point object

	Matrix3D invM; // inverse matrix object

	invM = M.inverse(); // calls the inverse calculation

	for (int i = 0; i < yMax; i++) {
		for (int j = 0; j < xMax; j++) {
	
			newX = j - xAdjust; // set x value to non-adjusted position
			newY = i - yAdjust; // set y value to non-adjusted position

			Point.y = newY; // set value to vector
			Point.x = newX; // set value to vector

			Point = invM * Point; // multiply by inverse matrix

			newX = newX + xAdjust; // adjust back to to positive position
			newY = newY + yAdjust; // adjust back to to positive position

			xVal = static_cast<int>(Point.x); // set values for source image
			yVal = static_cast<int>(Point.y); // set values for source image

			// if source value is out of range, set new image value to black
			if ((xVal < 0) || (xVal >= xres) || (yVal < 0) || ( yVal >= yres)) {
					newMap[newY][newX*4] = 0;
			} else {
				// transfer image information over to new pixmap
				newMap[newY][newX*4] = pixmap[yVal][xVal*4];
				// transfer image information over to new pixmap for G,B,and A values too
				for (int k = 1; k < 4; k++) {
					newMap[newY][newX*4+k] = pixmap[yVal][xVal*4+k];
				}
			}
		}
	}
}


/*
	This function loops to create the panning effect by starting to increasingly lower 
	scanlines into the display
*/
void panDown() {
	int frameCount = 0;

	for (int i = 0; i < 720; i++) {
		pan = ((displaySize) - (i*lineSize));

		backDisplay();

		if (frameCount % 4 == 0) {
		
			displayImage();

			if (outSwitch == true) {
				danWrite();
			}		
		}
		
		frameCount++;
	}
}


/*
	function calls that provide ship movement
*/
void moveShips(Matrix3D &M) {

	swapPixmap(); // sends ship 2 info to control vector
	calcCorners(M); // sizes new vector
	clearNewMap(); // creates output vector
	invMap(M); // performs warping
	compImages(); // composits first image to background

}


/*
	This function positions ships initially and holds the transformation information 
	that controls their movement
*/
void setShips(Matrix3D &M) {

	int x;
	int y;
	float oneX = 720.0; 
	float oneY = 720.0; 
	float twoX = 720.0; 
	float twoY = 720.0; 
	float oneXs = 1.0; 
	float oneYs = 1.0; 
	float twoXs = 1.0; 
	float twoYs = 1.0; 

	Translate(M, oneX, oneY); // defines initial position for ship

	setPixmap(); // sends ship 1 info to control vector
	calcCorners(M); // sizes new vector
	setNewMap(); // creates output vector
	invMap(M); // performs warping
	compImages(); // composits first image to background

	updateDisplayMap(); // sends compositied image to display vector

	moveShips(M);

	displayImage(); // displays final composited image

	for (int i = 0; i < 225; i++) {

		M.setidentity();

		oneX = (oneX + (oneX * .005));
		oneY = (oneY + (oneY * .001));
		oneXs = (oneXs * .991);
		oneYs = (oneYs * .991);	

		Translate(M, oneX, oneY); // defines initial position for ship		
		Scale(M, oneXs, oneYs);

		moveShips(M);

		updateDisplayMap(); // sends compositied image to display vector

		if (i > 125) {
			M.setidentity();

			twoX = (twoX - (twoX * .004));
			twoY = (twoY - (twoY * .017));
			twoXs = (twoXs * .997);
			twoYs = (twoYs * .997);	

			Translate(M, twoX, twoY); // defines initial position for ship
			Scale(M, twoXs, twoYs);

			moveShips(M);

		} else {
			swapPixmap();
		}

		displayImage(); // displays final composited image

		if (outSwitch == true) {
				danWrite();
		}		

		pan = ((displaySize) - (719*lineSize)); // reset pan value for later use
		backDisplay(); // removes ships from display
		updateDisplayMap(); // resets the display map
	}
}


/*
	this section gathers the filename information from the user in the terminal
*/
void openingMenu(Matrix3D &M) {
	
	string backName; // background file name
	string imgOneName; // first composite image file name
	string imgTwoName; // second composite image file name

	std::cout << "\nPlease enter the file name for the background image (1280 x 1440)\n or enter 0 for Tatooine: " << std::endl;
	
	std::cin >> backName;

	if (backName != "0") {
		infilename = backName;
	}
	if (backName == "0") {
		infilename = "sourceImages/back.jpg";
	}

	readScanline(); // read in the image
	setBack();
	
	if (cin) {
			cout << "\nNow we need a hero ship. Any green screen image will do (1280 x 720)\n or enter 0 for a Corellian Corvette: \n";
		}
		else {
			cerr << "\nI felt a great disturbance in the force...I fear something terrible has happened.\n";
			cin.clear();
		}

	std::cin >> imgOneName;

	if (imgOneName != "0") {
		infilename = imgOneName;
	}
	if (imgOneName == "0") {
		infilename = "sourceImages/corcor.jpg";
	}

	readScanline(); // read in the image
	createMask();
	setImgOne();

	if (cin) {
			cout << "\nOk, now we need a villain ship. Any green screen image will do (1280 x 720)\n or enter 0 for a Star Destroyer: \n";
	} else {
			cerr << "\nI felt a great disturbance in the force...I fear something terrible has happened.\n";
			cin.clear();
	}

	std::cin >> imgTwoName;

	if (imgTwoName != "0") {
		infilename = imgTwoName;
	}
	if (imgTwoName == "0") {
		infilename = "sourceImages/stard.jpg";
	}

	readScanline(); // read in the image
	createMask();
	setImgTwo();

	if (cin) {
			cout << "\nThe Force is strong with this one. Would you like to write your files out after compositing?\nIf so enter a one word filename base for output, if not, enter 0: ";
	} else {
			cerr << "\nI felt a great disturbance in the force...I fear something terrible has happened.\n";
			cin.clear();
	}

	std::cin >> userName;

	if (userName != "0") {
		std::cout << "\nPatience you must have, slow this program might run when printing these files." << std::endl;
		outSwitch = true;
	}

	if (userName == "0") {
		std::cout << "\nRoger that red leader!" << std::endl;
	}
		
	panDown();  // initiates the opening pan down sequence

	setDisplayMap(); // copies background image to the displayData vector for compositing

	setShips(M); // positions ships and initiates movement
}


/*
		Switch to set key functions
*/
void handleKey(unsigned char key, int x, int y){
  
  switch(key){

		case 'm':
		case 'M': 
			openingMenu(M);
      break;

    case 'q':		// q - quit
    case 'Q':
    case 27:		// esc - quit
      exit(0);
      
    default:		// not a valid key -- just ignore it
      return;
  }
}


/*
   Reshape Callback Routine: sets up the viewport and drawing coordinates
   This routine is called when the window is created and every time the window
   is resized, by the program or by the user
*/
void handleReshape(int w, int h){
  // set the viewport to be the entire window
  glViewport(0, 0, w, h);
  
  // define the drawing coordinate system on the viewport
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);

}


/*
   Main program to initiate glut utilities, windows, and load initial images 
	 if required
*/
int main(int argc, char* argv[]){
 
  // start up the glut utilities
  glutInit(&argc, argv);
  
  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(1280, 720);
  glutCreateWindow("Dan's Star Wars Compositer!");

	infilename = "sourceImages/opening.jpg"; // Loads our opening image filename
	readScanline(); // read in the opening image

	newdisplay = new unsigned char[xres*yres*4]; // create display vector

	toDisplay(); // display opening image

  // set up the callback routines to be called when glutMainLoop() detects
  // an event
  glutDisplayFunc(displayImage);	  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback

  // Routine that loops forever looking for events. It calls the registered
  // callback routine to handle each event that is detected
  glutMainLoop();
  return 0;
}
