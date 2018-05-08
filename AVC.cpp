#include <stdio.h>
#include <time.h>
#include "E101.h"

int followLine();
int followMaze();
int findCurveError();

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
	
	int error = findCurveError();
	
	printf("%d", error);
	
	return 0;
}

int findCurveError(){
	
	//stores how far to the left or right the line is
	int error = 0;
	int numberWhites = 0;
	
	//take picture
	take_picture();
	//set row to scan
	int scan_row = 120;
	//max and min white value on pixel
	int max = 0;
    int min =255;
    
    //for all pixels on row, find the max and min white value
   	for(int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
        //get max if larger
        if( pix > max){
			max = pix;
		}
		//get min if smaller
		if(pix < min){
			min =pix;
		}
    }
    //set threshold of what consitutes a white pixel by average of max and min
    int threshold = (max+min)/2;
    
    printf(" min=%d max=%d threshold=%d\n", min, max,threshold); //testing for finding min/max/threshold
       
    int white = 0;//white value (100 or 200)
    
    //go through all pixels. if below threshold its not a white pixel. if above then it is white
    for(int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
		if( pix > threshold){ //pixel is 'white'
			//white value of pixel is 100 (not white) and don't add to number of white pixels
			white = 200;
			numberWhites += 1;
			
		}else{ //pixel is 'black'
			//white value of pixel set to 200 and add to number of white pixels
			white = 100;
			
		}
		//add to error the weight and value of pixel (distance from center and whiteness)
		error = error + (i - 160)*white;
		
    }
    printf("\n");
	
	//normalising error for number of pixels
	error = error / numberWhites;
	
	return error;
}

int followMaze(){
	
	redCount ++; //test to move on
	
	return 0;
}
