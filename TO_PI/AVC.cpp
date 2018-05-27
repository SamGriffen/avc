#include <stdio.h>
#include <sys/time.h>
#include "E101.h"

int stage = 0; //what stage the bot is in
int wallClose = 200; //200 is test value change later. constant for what a close wall reads

//fields holding the max/min whiteness of pixels in a row
int max = 0;
int min = 255;
int upperIntensityMargin = 150; //margin of error for if line all white. will need fine tuning
int lowerIntensityMargin = 50; //margin of error for if line all black
int upperIndex = 320;
int lowerIndex = 0;

int minThreshold = 80;
int maxThreshold = 160;

// Array to store the line as a sequence of 1 and 0
int whiteArray[320];

// Stores number of pixels that determines an "all white" line
int allWhiteCount = 200;

// Flag representing whether to log to file or not
bool dev = true;

// Declare pins for IR sensors
int left_ir = 0;
int mid_ir = 1;
int right_ir = 2;

// Varialbes for PID control
unsigned char v_go = 40;
// double kp = 0.25;
double kp = 0.25;
double kd = 0.45;
double ki = 0.000;

// Stores total error
int total_error = 0;

// Stores the last error
int prevError = 0;

// Stores the last time
struct timeval last_time;

// Stores current time
struct timeval current_time;

// Stores the file
FILE *file;

int findMinMax(int scan_row);
int findCurveError(int threshold);

// Method for filling the whiteArray array of white/black for a given row, given a threshold value. Also returns the number of white pixels
int scanRow(int scan_row, int threshold);

void followLine(int error);
int followMaze();
void openGate();
//void changeWhitenessArray(int scan_row);
void curveyLineHandler(int scan_row);
void tapeMazeHandler(int scan_row);

int main(){
	init();

	// Open a file for logging
	file = fopen("log.txt", "w");

	// Define the row to scan on
	int scan_row = 120;

	try{
		openGate();
		// sleep1(5,0);

		//run line
		while(stage == 0){
			curveyLineHandler(scan_row);
		}


		//run line maze
		while(stage == 1){
			//tapeMazeHandler(error, scan_row);
			//fprintf(file, "THe 3rd quadrant has been reached\n");
			tapeMazeHandler(scan_row);
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

void curveyLineHandler(int scan_row){
	// Take picture for this loop of logic
	take_picture();
	// Find the threshold for black/white differentiating
	// int threshold = findMinMax(scan_row);

	int threshold = 120;

	// Populate the white array, and get the number of white pixels
	int numberWhites = scanRow(scan_row, threshold);

	int error = findCurveError(threshold);//find new error

	if(dev){
		fprintf(file, "Error: %d  NOW: %d  Threshold: %d\n",error, numberWhites, threshold);;
	}

	//if all pixels black, back up the vehicle
	// TODO figure out this value
	if(numberWhites < 10){
		set_motor(1, -1 * v_go);
		set_motor(2, -1 * v_go);
		fprintf(file, " line was all black, linewhiteness/threshold: %d\n", threshold);
	}
	//if all pixels are white, move onto the next stage
	// TODO tune this value
	else if(numberWhites > allWhiteCount){
		stage ++;
		fprintf(file, " moved to next stage linewhiteness/threshold: %d\n", threshold);
	}
	//if mix of pixels, keep following the line
	else{
		followLine(error);
	}

	//reset min and max
	min = 255;
	max = 0;
}

void tapeMazeHandler(int scan_row){
	take_picture();

	int threshold = 120;

	int numberWhites = scanRow(scan_row, threshold);


	///take scan
	//find max and min values of whiteness on the scan line
	upperIndex = 320;
	lowerIndex = 0;
	//reset min and max
	min = 255;
	max = 0;
	// int threshold = findMinMax(scan_row);

	///if all white
		///move bot forwards
	if(numberWhites > allWhiteCount){
		set_motor(1, v_go);
		set_motor(2, v_go);
	}

	///else if all black
		///turn bot hard left
	else if(numberWhites < 10){
		set_motor(1, 0);
		set_motor(2, v_go);
	}

	///else must be a mix of black and white
		///scan left side of scan line
		///if all white
			///take another scan further up
			///if line further up go straight else turn left
	else{
		upperIndex = 160;
		lowerIndex = 0;
		//reset min and max
		min = 255;
		max = 0;
		// threshold = findMinMax(scan_row);

		// If all the pixels are white
		if(numberWhites > allWhiteCount){
			scan_row = 220;
			// threshold = findMinMax(scan_row);

			// If it is all black
			if(numberWhites < 10){
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
		upperIndex = 320;
		lowerIndex = 160;
		//reset min and max
		min = 255;
		max = 0;
		// threshold = findMinMax(scan_row);

		// If all the pixels are white
		if(numberWhites > allWhiteCount){
			scan_row = 220;
			min = 255;
			max = 0;
			// threshold = findMinMax(scan_row);

			// If they are all black
			if(numberWhites < 10){
				set_motor(1, v_go);
				set_motor(2, v_go);
			}else{
				set_motor(1, v_go);
				set_motor(2, 0);
			}
			scan_row = 120;

			if(dev){
				printf("lineWhiteness/threshold: %d, right all white \n", threshold);
			}
		}

		///else no big turns needed
			///make sure parallel to line, same method as curved method
		else{
			upperIndex = 320;
			lowerIndex = 0;
			int error = findCurveError(threshold);
			followLine(error);
			//reset min and max
			min = 255;
			max = 0;
			if(dev){
				printf("adjusting to line \n");
			}
		}

	}
}

/*
void changeWhitenessArray(int scan_row){
	whitenessOfScans -= averageWhiteArray[averageCounter];
	averageWhiteArray[averageCounter] = findMinMax(scan_row);
	whitenessOfScans += averageWhiteArray[averageCounter];
	normalWhiteness = whitenessOfScans/100;
	averageCounter ++;
	if(averageCounter == 100){
		averageCounter = 0;
	}
	fprintf(file, "normalWhiteness: %d \n", normalWhiteness);
}
*/

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
 	for(int i = lowerIndex; i <upperIndex;i++){
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

// Method that takes a row to scan, and a threshold value to work with, and returns the numer of white pixels in that scan row. Also populates the whiteArray
int scanRow(int scan_row, int threshold){
	// Initialize the variable to store number of whites in
	int numberWhites = 0;

	//go through all pixels. if below threshold its not a white pixel. if above then it is white
	for(int i = 0; i <320;i++){
		int pix = get_pixel(scan_row,i,3);
		// printf("%d\n", pix);
		if(pix > threshold){
			// Increment the number of white pixels
			numberWhites += 1;
			// Add the pixel to the white array
			whiteArray[i] = 1;
		}
		else{
			// Set this pixel in the whitearray to 0
			whiteArray[i] = 0;
		}
	}

	return numberWhites;
}

//find direction error when following line
int findCurveError(int threshold){
	//stores how far to the left or right the line is
	int error = 0;
	int numberWhites = 0;

  //go through all pixels. if below threshold its not a white pixel. if above then it is white
  for(int i = 0; i <320;i++){
		int pix = whiteArray[i];

		numberWhites += pix;
		//add to error the weight and value of pixel (distance from center and whiteness)
		error += (i - 160)*pix;
  }

	//normalising error for number of pixels
	if(numberWhites != 0){
		error = error / numberWhites;
	}

	return error;
}

//following line
void followLine(int error){
	total_error += error;

	gettimeofday(&current_time, 0);

	int errorDifference = (error - prevError)/( (current_time.tv_sec - last_time.tv_sec)*1000000+(current_time.tv_usec-last_time.tv_usec));

	last_time = current_time;

	int dv = (double)error * kp + (double)errorDifference * kd + (double)total_error * ki; //

	int v_left = v_go + dv;
	int v_right = v_go - dv;

	if(dev){
		fprintf(file, "P Action: %d  I Action: %d  D Action: %d\n", (int)((double)error * kp),(int)((double)total_error * ki),(int)((double)errorDifference * kd));
		fprintf(file, "Left: %d Right: %d\n\n", v_left, v_right);
	}

	set_motor(1, v_left);
	set_motor(2, v_right);

	prevError = error; // Store the current error as the previous error
}

//following maze
void wallMazeHandler(){
	int left = read_anal og(left_ir);
	int right = read_analog(right_ir);
	wallMazeStraight(right,left);
}

void wallMazeStraight (int right, int left){
	int dv = wallMazeOffset(right, left);
	if(dev){
		//fprintf(file, "v_go: %d  dv: %d\n", v_go, dv);
	}
	set_motor(1,v_go+dv);
	set_motor(2,v_go-dv);
}

int wallMazeOffset(int right, int left){
	int error = (left-right);
	return ((error*maze_kp)+(error*maze_ki)*(error*maze_kd));
	if(dev){

	}
}
