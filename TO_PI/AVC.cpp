#include <stdio.h>
#include <time.h>
#include "E101.h"

int stage = 0; //what stage the bot is in
int wallClose = 200; //200 is test value change later. constant for what a close wall reads

//fields holding the max/min whiteness of pixels in a row
int max = 0;
int min = 255;

//with a given row to scan, find the min and max whiteness valuess
int findMinMax(int scan_row){
	take_picture();
    
   	for(int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
        //set max if larger
        if( pix > max){
			max = pix;
		}
		//set min if smaller
		if(pix < min){
			min =pix;
		}
    }
    
    //set threshold of what consitutes a white pixel by average of max and min
    int threshold = (max+min)/2;
    
	return threshold;
}

//find direction error when following line
int findCurveError(int scan_row, int threshold){
	//stores how far to the left or right the line is
	int error = 0;
	int numberWhites = 0;
    
    printf(" min=%d max=%d\n", min, max); //testing for finding min/max/threshold
       
    int white = 0;//white value (0 or 1)
    
    //go through all pixels. if below threshold its not a white pixel. if above then it is white
    for(int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
		if( pix > threshold){ //pixel is 'white'. add to number of white pixels and whiteness is 100
			white = 100;
			numberWhites += 1;
			
		}else{ //pixel is 'black'. don't add to number of white pixels. whiteness is 0
			white = 0;
		}
		//add to error the weight and value of pixel (distance from center and whiteness)
		error = error + (i - 160)*white;
    }
	
	//normalising error for number of pixels
	if(numberWhites != 0){
		error = error / numberWhites;
	}
	
	return error;
}


//following line
int followLine(int error, int scan_row, int threshold){
	
	//create variables for PID control
	unsigned char v_go = 40;
	double kp = 0.0015;
	double kd = 0.00;
	//double black;
	
	int prevError = error; //store previous error
	error = findCurveError(scan_row, threshold);//find new error
	
	int errorDifference = error - prevError;
	
	int dv = (double)error * kp + (double)errorDifference * kd; //
	
	printf("dv: %d\n", dv);
	
	int v_left = v_go + dv;
	int v_right = v_go - dv;
	
	set_motor(1, v_left);
	set_motor(2, v_right);
	
	printf("error: %d", error);
	
	return error;
}


//following maze
int followMaze(){
	
	stage ++; //test to move on
	
	return 0;
}

int main(){
	init();	
	
	/*
	//set up network variables controlling message sent/received and if connected or not
	char message[24] = "Please";
	int port = 1024;
	char serverAddress[15] = {"130.195.6.196"};
	unsigned char successful = 1;
	
	//while not connected, try to connect
	while(successful != 0){
		successful = connect_to_server(serverAddress, port);
	}
	successful = 1;
	
	//try to send to server until successfully sent
	while(successful != 0){
		printf("\n before send: " + message[24]);
		successful = send_to_server(message);
		printf("\n after send: " + message[24]);
	}
	
	//try to reciebve message until successfully recieved
	while(successful != 0){
		printf("\n before recienve: " + message[24]);
		successful = receive_from_server(message);
		printf("\n after recieve: " + message[24]);
	}
	successful = 1;
	
	//try to send to server until successfully sent
	while(successful != 0){
		printf("\n before send2: " + message[24]);
		successful = send_to_server(message);
		printf("\n after send2: " + message[24]);
	}
	*/
	
	try{
		//values for what constitues an entiraly white or black line
		int allWhite = 120;
		int allBlack = 90;
		
		//run line
		int scan_row = 120;
		int error = 0;
		while(stage == 0){
			
			//find max and min values of whiteness on the scan line
			int threshold = findMinMax(scan_row);
			printf("threshold: %d", threshold);
			
			//if all pixels black, back up the vehicle
			if(max < allBlack){
				printf("All black \n");
				set_motor(1, 0);
				set_motor(2, 0);
				
			//if all pixels are white, move onto the next stage
			}else if(min > allWhite){
				printf("All white \n");
				set_motor(1, 0);
				set_motor(2, 0);
				
			//if mix of pixels, keep following the line
			}else{
				error = followLine(error, scan_row, threshold);
				//reset min and max
				min = 255;
				max = 0;
			}
		}
		
		//run maze
		while(stage == 1 || stage == 2){
			followMaze();
		}
		
		//if program finishes, turn off the motors
		set_motor(1,0);
		set_motor(2,0);
	
	//if program crashes, turn off the motors
	}catch(long){
		set_motor(1,0);
		set_motor(2,0);}
	return 0;
}
