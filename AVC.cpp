#include <stdio.h>
#include <time.h>
#include "E101.h"

int followLine();
int followMaze();

int redCount = 0; //
int wallClose = 200; //200 is test value change later. constant for what a close wall reads

int main(){
	init();
	
	while(redCount == 0){
		followLine();
	}
	while(redCount == 1 || redCount == 2){
		followMaze();
	}
	
	set_motor(1,0);
	set_motor(2,0);
	return 0;
}

int followLine(){
	
	//take picture
	take_picture();
	//set row to scan
	int scan_row = 120;
	//max and min white value on pixel
	int max = 0;
    int min =255;
    
    //for all pixels on row, find the max and min white value
   	for (int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
        //get max if larger
        if ( pix > max){
			max = pix;
		}
		//get min if smaller
		if (pix < min){
			min =pix;
		}
    }
    //set threshold of what consitutes a white pixel by average of max and min
    int threshold = (max+min)/2;
    
    printf(" min=%d max=%d threshold=%d\n", min, max,threshold); //testing for finding min/max/threshold
    
    //create array holding boolean value stating whether a pixel is white or not, for all pixels
    int white[320];
    //go through all pixels. if below threshold its not a white pixel. if above then it is.
    for (int i = 0; i <320;i++){
		white[i]= 0 ;
		int pix = get_pixel(scan_row,i,3);
		if ( pix > threshold){
			white[i] = 1;
		}
    }
    
    for (int i = 0; i <320;i++){
		printf("%d ",white[i]); //print if pixel was white or not, for each pixel.
    }
    printf("\n");
	
	redCount ++; //test to move on
	
	return 0;
}

int followMaze(){
	
	redCount ++; //test to move on
	
	return 0;
}
