#include <stdlib.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <cmath>
#include <sstream>

using namespace std;

/*
pixelArray:	3-D array for storing 2D pixel data/color table of RGB planes
extra:	array for storing data between Bitmap header and offset/pixel data
footer:	array for storing extra data after saving/reading pixel data (if any)
*/
unsigned char ***pixelArray, *extra, *footer;
//	Using pragma for allowing structure to have 2 byte segmented
//	structure. Otherwise, it will have 4 byte segmented structure
#pragma pack(2)


//	Header for Bitmap file, contains 2 parts: Bitmap File Header, Bitmap Info Headr
typedef struct BMPHEADER{
	// File Header
	uint16_t	type; // 2 bytes :'BM' for bitmap file
	uint32_t	size; // 4 bytes : File size in bytes
	uint16_t	reserved1; // 2 bytes : Reserved space, not for editing
	uint16_t	reserved2; // 2 bytes : Reserved space, not for editing
	uint32_t	offset;	 // 4 bytes : Address at which pixel data srarts in file

	// Info Header
	uint32_t	infoHeaderSize; // 4 bytes : Size of File info header in bytes
	uint32_t	width; // 4 bytes : Width of the image in pixels
	uint32_t	height; // 4 bytes : Height of the image in pixels
	uint16_t	colorPlanes; // 2 bytes : Number of color planes, fixed value 1 (not to be edited by user)
	uint16_t	bpp; // 2 bytes : Bits per pixel; for this program, 8bpp means 1 channel and 24bpp means 3 channel
	uint32_t	compression; // 4 bytes : Compression method (not to be edited by user)
	uint32_t	imageSize; // 4 bytes : 	the image size. This is the size of the raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps (not to be edited by user)
	uint32_t	horizontalResolution; // 4 bytes : the horizontal resolution of the image. (pixel per meter, signed integer)
	uint32_t	verticalResolution; // 4 bytes : 	the vertical resolution of the image. (pixel per meter, signed integer)
	uint32_t	numColor; // 4 bytes : the number of colors in the color palette, or 0 to default to 2^n (not to be edited by user)
	uint32_t	imptColor; // 4 bytes : the number of important colors used, or 0 when every color is important (not to be edited by user)

}BMPHEADER;

#pragma pack()


/*
printInfo( BMPHEADER header)
Description:	function for printing information about file using header
Input:			header:	Header of the image
Output:			No explicit output, prints output on console
*/
void printInfo(BMPHEADER header){

	cout << "Type: " << (char)(header.type) << (char)(header.type >> 8) << endl;
	cout << "size: " << header.size << endl;
	cout << "offset: " << header.offset << endl;
	cout << "Info header size: " << header.infoHeaderSize << endl;
	cout << "Bitmap width: " << header.width << endl;
	cout << "Bitmap height " << header.height << endl;
	cout << "Color planes: " << header.colorPlanes << endl;
	cout << "BPP: " << header.bpp << endl;
	cout << "Compression Method " << header.compression << endl;
	cout << "Raw bmp data size: " << header.imageSize << endl;
	cout << "Horizontal Resolution: " << header.horizontalResolution << endl;
	cout << "Vertical Resolution: " << header.verticalResolution << endl;
	cout << "No of Color: " << header.numColor << endl;
	cout << "No of imp colors: " << header.imptColor << endl;
}

/*
ReadBMP(string inputFile)
Dscription:		Function to read header and data from the Bitmap file
Input:			inputFile:	Name of the Bitmap file to be read
Output:			header of the Bitmap file (also loads data in a dynamically allocated array)
*/
BMPHEADER ReadBMP(string inputFile){

	FILE *fptr;	// File pointer
	BMPHEADER header = BMPHEADER();	// File and Info header
	int paddingSize, channel;
	//	paddingSize:	Width of the image (including number of bits to be padded)
	//	channel:	Number of channel: 1 for grayscale, 3 for RGB

	//	Reading the file
	fopen_s(&fptr, inputFile.c_str(), "rb");
	cout << "FileName : " << inputFile << endl;

	//	If error occured in opening file
	if (fptr == NULL){
		cout << "File Not found" << endl;
		return header;
	}

	//	Reading the header data
	fread(&header, sizeof(unsigned char), sizeof(BMPHEADER), fptr);

	//	Displaying the information from header
	printInfo(header);

	//	Number of channels
	channel = header.bpp / 8;

	//	Reading the extra data stored between header and pixel data/color table
	extra = new unsigned char[header.offset - sizeof(BMPHEADER)];
	fread(&extra, sizeof(unsigned char), header.offset - sizeof(BMPHEADER), fptr);

	// Reading the pixel data/color table
	//	Step 1: Allocating size
	pixelArray = new unsigned char**[channel];
	for (int i = 0; i < 3; i++){
		pixelArray[i] = new unsigned char*[header.height];

		for (int j = 0; j < header.height; j++)
			pixelArray[i][j] = new unsigned char[header.width];
	}

	// Calculating width/row size with padded size
	paddingSize = header.width * channel;

	while (paddingSize % 4)
		paddingSize++;

	//	Step 2:	Reading the pixel data/color table

	//	In Bitmap file, data is stored starting from bottom-to-top, left-to-right
	for (int i = header.height - 1; i >= 0; i--){
		//	Going to the start of each row
		fseek(fptr, header.offset + paddingSize*i, SEEK_SET);

		//	Reading the pixel data/color table
		for (int j = 0; j < header.width; j++){
			for (int k = channel - 1; k >= 0; k--){
				fread(&(pixelArray[k][i][j]), sizeof(unsigned char), sizeof(unsigned char), fptr);
				//cout<< i << " "<< j << " " << k <<endl;
			}
		}
	}

	//	Adding data from image/color table
	fread(&footer, sizeof(unsigned char), header.size - header.offset - (channel * header.width * header.height), fptr);

	//	Closing the file
	fclose(fptr);



	//	Returning the header
	return header;
}

/*
ConvertFlipGrayscale( BMPHEADER header)
Description:	Function to convert the color image to grayscale and then flip the grayscale along the diagonal
Input:			header:	Header of the file to be converted to grayscale and flipped
Output:			array which contains data for grayscale converted
and flipped image
*/

unsigned char***TransformBMP(BMPHEADER* header, int action){

	int channel, tempSwap, gray;
	//	channel:	Color Depth, 1 for grayscale, 3 for RGB
	//	tempSwap:	variable to	help in the swapping for flipping (not being used here)
	//	gray:		varible for storing grayscale value
	unsigned char temp;	//	variable for helping in swapping value of width and height in header

	//	Finding number of channels/Color depth, 1 means 8bpp/grayscale, 3 means 24bpp/RGB color image
	channel = header->bpp / 8;

	// for(int i = 0; i < header.height; i++)
	// 	for(int j = 0; j < header.width; j++)
	// 		cout<< pixelArray[i][j] <<endl;

	// New array for storing grayscale converted and flipped image
	unsigned char ***newPixelArray = new unsigned char**[channel];

	// Array dimension for 45 degree rotation
	uint32_t length45 = (header->width + header->height - 1);


	// Grayscale conversion for RGB color image
	if (channel == 3)
		for (int i = 0; i < header->height; i++){
			for (int j = 0; j < header->width; j++){

				// Grayscale data = R * 0.30 + G * 0.59 + B * 0.11
				gray = (pixelArray[0][i][j] * 0.30) + (pixelArray[1][i][j] * 0.59) + (pixelArray[2][i][j] * 0.11);

				//	Giving same value to all channels
				pixelArray[0][i][j] = gray;
				pixelArray[1][i][j] = gray;
				pixelArray[2][i][j] = gray;
			}
		}

	//Gray Scaled image

	if(action == 0){
		header->offset = 1078;
		header->bpp = 8;

		// Returning the gray scale array
		
		return pixelArray;
	}

	// Flipping pixel data/color table along the diagonal
	else if(action == 1){
		for (int i = 0; i < channel; i++){
			newPixelArray[i] = new unsigned char*[header->width];

			for (int j = 0; j < header->width; j++)
				newPixelArray[i][j] = new unsigned char[header->height];
		}

		//	Flipping the image
		for (int i = 0; i < header->height; i++)
			for (int j = 0; j < header->width; j++)
				newPixelArray[0][j][i] = pixelArray[0][header->height - i - 1][header->width - j - 1];

	}


	// 90 degree rotation anticlockwise
	else if(action == 2){
		for (int i = 0; i < channel; i++){
			newPixelArray[i] = new unsigned char*[header->width];

			for (int j = 0; j < header->width; j++)
				newPixelArray[i][j] = new unsigned char[header->height];
		}

		for (int i = 0; i < header->height; i++)
			for (int j = 0; j < header->width; j++){
						newPixelArray[0][j][-(i- header->height) - 1] = pixelArray[0][i][j];
			}
	}


	// 45 degree rotation anticlockwise
	else if(action == 3){
		
		for (int i = 0; i < channel; i++){
			newPixelArray[i] = new unsigned char*[length45];

			for (int j = 0; j < length45; j++)
				newPixelArray[i][j] = new unsigned char[length45];
		}

		int j = length45-header->width;
		for (int i = 0; i < header->height; i++){
			for (int k = 0; k < header->width; k++){
				newPixelArray[0][i+k][j+k] = pixelArray[0][i][k];
			}
			j--;
		}
		int m = header->height-1;
		for(int i = 1 ; i < header->height ; i++){
			int k = 0;
			for(int j = m; j < header->width + m -1 ; j++){
					newPixelArray[0][i+k][j] = newPixelArray[0][i+k-1][j];
				k++;
			}
			m--;
		}

		

	}

	//Scaling twice
	else if(action == 4){

		uint32_t newHeight = header->height*2;
		uint32_t newWidth = header->width*2;

		for (int i = 0; i < channel; i++){
			newPixelArray[i] = new unsigned char*[newHeight];

			for (int j = 0; j < newHeight; j++)
				newPixelArray[i][j] = new unsigned char[newWidth];
		}

		for (int i = 0; i < newHeight; i++){
			for (int j = 0; j < newWidth; j++)
				newPixelArray[0][i][j] = pixelArray[0][i/2][j/2];
		}
	}
	

	//	Assigning same value for all channels
	if (channel == 3){
		newPixelArray[1] = newPixelArray[0];
		newPixelArray[2] = newPixelArray[0];
	}

	//	Swapping value of the width and height

	if(action == 1 || action == 2){
		tempSwap = header->width;
		header->width = header->height;
		header->height = tempSwap;
	}

	else if(action == 3){
		header->width = length45;
		header->height = length45;
	}

	else if(action == 4){
		header->width = (header->width)*2;
		header->height = (header->height)*2;
	}
	

	header->offset = 1078;
	header->bpp = 8;

	// Returning the pixel array
	
	return newPixelArray;
}


/*
WriteBMP(string outFile, BMPHEADER header)
Description:	Function to write the image on the disk on machine
Input:			outFile, name of the output file where the data is to be written
header, header for the output file
Output:			No explicit output, Output is written in an image file on the disk
*/
void WriteBMP(string outFile, BMPHEADER header){
	FILE *fptr;		//File pointer
	int channel, paddingSize;

	//	channel:	Number of channel: 1 for grayscale, 3 for RGB
	//	paddingSize:	Width of the image (including number of bits to be padded)

	//	Finding number of channels/Color depth, 1 means 8bpp/grayscale, 3 means 24bpp/RGB color image
	channel = header.bpp / 8;



	//	opening the Bitmap file
	outFile = outFile + ".bmp";
	fopen_s(&fptr, outFile.c_str(), "wb");

	//	If error occured in opening file
	if (fptr == NULL){
		cout << "Error in opening file" << endl;
		return;
	}

	//	Writing header to the file
	fwrite(&header, sizeof(unsigned char), sizeof(BMPHEADER), fptr);
	//	Writing extra data (that lies between end of headr and offset) to the file
	int k = 0;
	for (int i = 0; i < 256; i++){
		for (int j = 0; j < 3; j++)
			fwrite(&i, sizeof(unsigned char), sizeof(unsigned char), fptr);
		fwrite(&k, sizeof(unsigned char), sizeof(unsigned char), fptr);
	}

	//fwrite( &extra, sizeof(unsigned char),header.offset - sizeof(BMPHEADER), fptr);

	// Finding the image width/row size including the number of bytes to be padded
	paddingSize = header.width * channel;

	while (paddingSize % 4)
		paddingSize++;


	//	Writing the pixel data/color data to the image
	//	In Bitmap file, data is stored starting from bottom-to-top, left-to-right
	for (int h = header.height - 1; h >= 0; h--)
	{
		//	Going to the start of each row
		fseek(fptr, header.offset + paddingSize*h, SEEK_SET);

		//	Writing data to rows in each channel
		for (int w = 0; w < header.width; w++){
			//	Writing data in each channel by iterating
			for (int k = channel - 1; k >= 0; k--)
				fwrite(&(pixelArray[k][h][w]), sizeof(unsigned char), sizeof(unsigned char), fptr);
		}
	}

	//	Writing extra data (if any, from original image ), after the color table
	//fwrite( &footer, sizeof(unsigned char), header.size - header.offset - ( channel * header.width * header.height ), fptr );

	//	Closing the file
	fclose(fptr);

}


int main() {

	string fileName;	// 	Input file name
	BMPHEADER myHeader;	//	File header
	int action;

	//	Taking name of the input Bitmap file
	cout << "Please provide the name of the input file (only Bitmap file) :" << endl;
	cout << "Example: cameraman" << endl;
	cin >> fileName;


	cout << "Please provide the required action integer: 0. Gray Scale Only , 1. Flip Diagonally , 2. Rotate 90 degrees , 3. Rotate 45 degrees , 4. Scale twice" << endl;
	cin >> action;

	string outputFile = "outImg_" + to_string(action) + fileName;

	//	Location of the image
	fileName = "./input_images/" + fileName + ".bmp";

	//	Reading File
	myHeader = ReadBMP(fileName);
	//	Grayscale conversion and Flipping
	pixelArray = TransformBMP(&myHeader, action);
	//	Writing to the disk
	// string outputFile = "outputImg" + to_string(action) + fileName;
	WriteBMP(outputFile, myHeader);

	printInfo(myHeader);
	cout << "Execution Completed" << endl;
	// For observing output on the console
	_getch();

	return 0;
}
