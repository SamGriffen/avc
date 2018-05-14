#include <stdio.h>
#include <sys/time.h>
#include "E101.h"

int stage = 0; //what stage the bot is in
int wallClose = 200; //200 is test value change later. constant for what a close wall reads

//fields holding the max/min whiteness of pixels in a row
int max = 0;
int min = 255;

// Flag representing whether to log to file or not
bool dev = true;

// Declare pins for IR sensors
int left_ir = 0;
int mid_ir = 1;
int right_ir = 3;

// Varialbes for PID control
unsigned char v_go = 40;
double kp = 0.2;
double kd = 0.00;

// Stores the last time
struct timeval last_time;

// Stores current time
struct timeval current_time;

// Stores the file
FILE *file;

int findMinMax(int scan_row);
int findCurveError(int scan_row, int threshold);
int followLine(int error, int scan_row, int threshold);
int followMaze();
void openGate();

int main(){
	init();

	// openGate();

	// Open a file for logging
	file = fopen("log.txt", "w");

	try{
		// openGate();

		//values for what constitues an entiraly white or black line
		int allWhite = 120;
		int allBlack = 90;

		int v_go = 40;

		//run line
		int scan_row = 120;
		int error = 0;
		while(stage == 0){

			//find max and min values of whiteness on the scan line
			int threshold = findMinMax(scan_row);
			if(dev){
				fprintf(file, "threshold: %d", threshold);;
			}


			//if all pixels black, back up the vehicle
			if(max < allBlack){
				set_motor(1, -1 * v_go);
				set_motor(2, -1 * v_go);

			//if all pixels are white, move onto the next stage
			}else if(min > allWhite){
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

void openGate(){
	//set up network variables controlling message sent/received and if connected or not
	char message[24] = "Please";
	int port = 1024;
	char serverAddress[15] = "130.195.6.196";
	unsigned char successful = 1;

	//while not connected, try to connect
	while(successful != 0){
		successful = connect_to_server(serverAddress, port);
	}
	successful = 1;

	//try to send to server until successfully sent
	while(successful != 0){
		successful = send_to_server(message);
	}

	//try to reciebve message until successfully recieved
	while(successful != 0){
		printf(message);
		successful = receive_from_server(message);
		printf(message);
	}
	successful = 1;

	//try to send to server until successfully sent
	while(successful != 0){
		printf(message);
		successful = send_to_server(message);
		printf(message);
	}
}

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

    int white = 0;//white value (0 or 1)

    //go through all pixels. if below threshold its not a white pixel. if above then it is white
    for(int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
		if( pix > threshold){ //pixel is 'white'. add to number of white pixels and whiteness is 100
			white = 1;
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
	//double black;

	int prevError = error; //store previous error
	error = findCurveError(scan_row, threshold);//find new error

	gettimeofday(&current_time, 0);

	int errorDifference = (error - prevError)/( (current_time.tv_sec - last_time.tv_sec)*1000000+(current_time.tv_usec-last_time.tv_usec));

	last_time = current_time;

	int dv = (double)error * kp + (double)errorDifference * kd; //

	int v_left = v_go + dv;
	int v_right = v_go - dv;

	set_motor(1, v_left);
	set_motor(2, v_right);

	return error;
}


//following maze
int followMaze(){

	stage ++; //test to move on

	return 0;
}
