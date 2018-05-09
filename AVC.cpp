#include <stdio.h>
#include <time.h>
#include "E101.h"

int stage = 0; //what stage the bot is in
int wallClose = 200; //200 is test value change later. constant for what a close wall reads

//find direction error when following line
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

//following line
int followLine(int error){
	
	//create variables for PID control
	unsigned char v_go = 40;
	double kp = 0.0025;
	double kd = 0.00;
	
	int prevError = error; //store previous error
	error = findCurveError();//find new error
	
	unsigned char errorDifference = error - prevError;
	
	int dv = (double)error * kp + (double)errorDifference * kd; //
	
	printf("dv: %d\n", dv);
	
	int v_left = v_go + dv;
	int v_right = v_go - dv;
	
	set_motor(1, v_left);
	set_motor(2, v_right);
	
	printf("%d", error);
	
	return error;
}

int followMaze(){
	
	stage ++; //test to move on
	
	return 0;
}

int main(){
	init();	
	//set up network variables controlling message sent/received and if connected or not
	//char message[24];
	//int port = 0;
	//char serverAddress[15] = {};
	//unsigned char successful = 1;
	
	//while not connected, try to connect
	//while(successful != 0){
		//successful = connect_to_server(serverAddress, port);
	//}
	//successful = 1;
	
	//try to reciebve message until successfully recieved
	//while(successful != 0){
		//receive_from_server(message);
	//}
	//successful = 1;
	
	//try to send to server until successfully sent
	//while(successful != 0){
		//send_to_server(message);
	//}
	
	//bot runs stage until end of stage, then moves to next stage
	int error = 0;
	while(stage == 0){
		error = followLine(error);		
		
	}
	while(stage == 1 || stage == 2){
		followMaze();
	}
	
	set_motor(1,0);
	set_motor(2,0);
	return 0;
}
