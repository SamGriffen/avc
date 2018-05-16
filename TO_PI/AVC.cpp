#include <stdio.h>
#include <sys/time.h>
#include "E101.h"

int stage = 0; //what stage the bot is in
int wallClose = 200; //200 is test value change later. constant for what a close wall reads

//fields holding the max/min whiteness of pixels in a row
int max = 0;
int min = 255;
int lineWhiteness = 0;
int halfLineWhiteness = 0;

// Flag representing whether to log to file or not
bool dev = true;

// Declare pins for IR sensors
int left_ir = 0;
int mid_ir = 1;
int right_ir = 3;

// Varialbes for PID control
unsigned char v_go = 40;
double kp = 0.25;
double kd = 0.45;
double ki = 0.0001;

int total_error = 0;

// Stores the last time
struct timeval last_time;

// Stores current time
struct timeval current_time;

// Stores the file
FILE *file;

int findMinMax(int scan_row);
int findLeftIntensity(int scan_row);
int findRightIntensity(int scan_row);
int findCurveError(int scan_row, int threshold);
int followLine(int error, int scan_row, int threshold);
int followMaze();
void openGate();

int main(){
	init();

	// Open a file for logging
	file = fopen("log.txt", "w");

	try{
		
		//find the average whiteness of a normal line
		int scan_row = 120;
		int intensityMargin = 80; //margin of error. will need fine tuning
		take_picture();
		int normalWhiteness = 0;
		for(int i = 0; i <320;i++){
			int pix = get_pixel(scan_row,i,3);
			normalWhiteness += pix;
		}
		
		openGate();
		// sleep1(5,0);

		int v_go = 40;

		//run line
		int error = 0;
		while(stage == 0){

			//find max and min values of whiteness on the scan line
			int threshold = findMinMax(scan_row);
			if(dev){
				fprintf(file, "threshold: %d", threshold);;
			}

			//if all pixels black, back up the vehicle
			if(lineWhiteness < normalWhiteness + intensityMargin){
				set_motor(1, -1 * v_go);
				set_motor(2, -1 * v_go);
				//reset min and max
				min = 255;
				max = 0;

			//if all pixels are white, move onto the next stage
			}else if(lineWhiteness > normalWhiteness - intensityMargin){
				stage += 1;
				//reset min and max
				min = 255;
				max = 0;

			//if mix of pixels, keep following the line
			}else{
				error = followLine(error, scan_row, threshold);
				//reset min and max
				min = 255;
				max = 0;
			}
		}
	
	
		//run line maze
		while(stage == 1){
			
			///take scan
			//find max and min values of whiteness on the scan line
			int threshold = findMinMax(scan_row);
			if(dev){
				fprintf(file, "threshold: %d", threshold);;
			}
	
			///if all white
				///move bot forwards
			if(halfLineWhiteness > normalWhiteness - intensityMargin){
				set_motor(1, v_go);
				set_motor(2, v_go);
			}
		
			///else if all black
				///turn bot hard left
			else if(lineWhiteness < normalWhiteness + intensityMargin){
				set_motor(1, 0);
				set_motor(2, v_go);
			}			
			
			///else must be a mix of black and white
				///scan left side of scan line
				///if all white
					///take another scan further up
					///if line further up go straight else turn left
			else{
				threshold = findLeftIntensity(scan_row);
				
				if(halfLineWhiteness > normalWhiteness/2 - intensityMargin){
					scan_row = 220;
					threshold = findMinMax(scan_row);
					if(lineWhiteness < normalWhiteness + intensityMargin){
						set_motor(1, v_go);
						set_motor(2, v_go);
					}else{
						set_motor(1, 0);
						set_motor(2, v_go);
					}
					scan_row = 120;
				}
				
			
				///scan right side of scan line
				///if all white
					///take another scan further up
					///if Line further up go straight else turn right
				threshold = findRightIntensity(scan_row);
				if(halfLineWhiteness > normalWhiteness/2 - intensityMargin){
					scan_row = 220;
					threshold = findMinMax(scan_row);
					if(lineWhiteness < normalWhiteness + intensityMargin){
						set_motor(1, v_go);
						set_motor(2, v_go);
					}else{
						set_motor(1, v_go);
						set_motor(2, 0);
					}
					scan_row = 120;
				}
				
				///else no big turns needed
					///make sure parallel to line, same method as curved method
				else{
					error = followLine(error, scan_row, threshold);
					//reset min and max
					min = 255;
					max = 0;
				}
					
			}
		}
		
		//run maze
		while(stage == 2){
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
	char message[24] = {"Please"};
	char password[24] = {};
	int port = 1024;
	char serverAddress[15] = "130.195.6.196";

	connect_to_server(serverAddress, port);

	send_to_server(message);

	receive_from_server(password);

	send_to_server(password);
}

//with a given row to scan, find the min and max whiteness valuess
int findMinMax(int scan_row){
	take_picture();
	
	//finds total whiteness in the line
	lineWhiteness = 0;

 	for(int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
		
		//add whiteness of pixel to total whiteness of line
		lineWhiteness += pix;
		
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

int findLeftIntensity(int scan_row){
	
	//finds total whiteness in half the line
	halfLineWhiteness = 0;
	
	for(int i = 0; i <160;i++){
		int pix = get_pixel(scan_row,i,3);
	
		//add whiteness of pixel to total whiteness of half the scanline
		halfLineWhiteness += pix;
	
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

int findRightIntensity(int scan_row){
	
	//finds total whiteness in half the line
	halfLineWhiteness = 0;
	
	for(int i = 160; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
	
		//add whiteness of pixel to total whiteness of half the scanline
		halfLineWhiteness += pix;
	
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

	total_error += error;

	gettimeofday(&current_time, 0);

	int errorDifference = (error - prevError)/( (current_time.tv_sec - last_time.tv_sec)*1000000+(current_time.tv_usec-last_time.tv_usec));

	last_time = current_time;

	int dv = (double)error * kp + (double)errorDifference * kd + (double)total_error * ki; //

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
