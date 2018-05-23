#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "E101.h"

int stage = 0; //what stage the bot is in

int minThreshold = 80;
int maxThreshold = 160;

int scanUpperLimit = 320;
int scanLowerLimit = 0;

// Array to store the line as a sequence of 1 and 0
int whiteArray[320];

// Stores number of pixels that determines an "all white" line
int allWhiteCount = 200;

// Flag representing whether to log to file or not
bool dev = false;

// Flag representing whether to log to a csv
bool csv_dev = true;

// Declare pins for IR sensors
int left_ir = 0;
int mid_ir = 1;
int right_ir = 2;

// Varialbes for PID control
unsigned char v_go = 45;
// double kp = 0.25;
double kp = 0.25;
double kd = 0.45;
double ki = 0.0005;

// For maze
double maze_kp = 0.25;
double maze_kd = 0.45;
double maze_ki = 0.000;

// Variable to store the minimum reading for a wall being "close"
int ir_close_reading = 130;

// Stores total error
int total_error = 0;
int prevError = 0;

// Stores the last time and current time
struct timeval last_time;
struct timeval current_time;

// Stores time at startup
struct timeval start_time;

// Stores the file
FILE *file;

FILE *csv_log;

int findMinMax(int scan_row);
int findCurveError(int threshold);

// Method for filling the whiteArray array of white/black for a given row, given a threshold value. Also returns the number of white pixels
int scanRow(int scan_row, int threshold);

void followLine(int error);
int followMaze();
void openGate();
void curveyLineHandler(int scan_row);
void tapeMazeHandler(int scan_row);
void wallMazeStraight(int right, int left);
int wallMazeOffset(int right, int left);
void wallMazeHandler();
bool scanRed(int scan_row);

int main(){
	init();

	gettimeofday(&start_time, 0);

	// Open a file for logging
	file = fopen("log.txt", "w");
	
	// Open a csv for logging
	csv_log = fopen("csv_log.csv", "w");

	fprintf(csv_log, "Error, Time\n");

	// Define the row to scan on
	int scan_row = 120;

	try{
		openGate();

		//run line
		while(stage == 0){
			curveyLineHandler(scan_row);
		}

		stage = 2;

		//run line maze
		while(stage == 1){
			//fprintf(file, "THe 3rd quadrant has been reached\n");
			tapeMazeHandler(scan_row);
		}

		//run maze
		while(stage == 2){
			wallMazeHandler();
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
	// Threshold for black/white differentiating
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

}

void tapeMazeHandler(int scan_row){
	take_picture();

	// Treshold for black/white screening
	int threshold = 120;
	scan_row = 120;

	// Populate the white array, and get the number of white pixels
	int numberWhites = scanRow(scan_row, threshold);

	// if most of scan white, move forwards
	if(numberWhites > allWhiteCount){
		set_motor(1, v_go);
		set_motor(2, v_go);
	}

	// else if most of scan black, turn left
	else if(numberWhites < 10){
		set_motor(1, 0);
		set_motor(2, v_go);
	}

	///else must be a mix of black and white
	else{

		//scan further ahead to see if can keep going straight
		scan_row = 220;
		numberWhites = scanRow(scan_row, threshold);
		scan_row = 120;

		//if there is a line ahead, try to follow it using error correction
		if(numberWhites > 10){
			int error = findCurveError(threshold);
			followLine(error);
			if(dev){
				printf("adjusting to line \n");
			}

		//isn't a line ahead so scan the left side for a turn
		}else{

			scanLowerLimit = 0;
			scanUpperLimit = 160;
			numberWhites = scanRow(scan_row, threshold);
			scanLowerLimit = 0;
			scanUpperLimit = 320;

			//if there is a line going to the left go left
			if(numberWhites > allWhiteCount/2){
				set_motor(1, 0);
				set_motor(2, v_go);
			}

			//otherwise turn right as can't go forwards or left
			else{
				set_motor(1, v_go);
				set_motor(2, 0);
			}
		}

	}
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

// Method that takes a row to scan, and a threshold value to work with, and returns the numer of white pixels in that scan row. Also populates the whiteArray
int scanRow(int scan_row, int threshold){
	// Initialize the variable to store number of whites in
	int numberWhites = 0;

	//go through all pixels. if below threshold its not a white pixel. if above then it is white
	for(int i = scanLowerLimit; i <scanUpperLimit;i++){
		int pix = get_pixel(scan_row,i,3);
		// printf("%d\n", pix);
		if(pix > threshold){
			// Increment the number of white pixels
			numberWhites += 1;
			// Add the pixel to the white array, value 1
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

	if(csv_dev){
		// Get the current time
	 	long csv_time = (current_time.tv_sec - start_time.tv_sec) * 1000 + (current_time.tv_usec - start_time.tv_usec) / 1000;
		fprintf(csv_log, "%d,%d\n", error, csv_time);
	}

	set_motor(1, v_left);
	set_motor(2, v_right);

	prevError = error; // Store the current error as the previous error
}


//following maze
void wallMazeHandler(){
	int left = read_analog(left_ir);
	int right = read_analog(right_ir);
	int front = read_analog(mid_ir);

	// If a wall is in front of the robot, don't crash
	if(front > ir_close_reading){
			// If the right side is clear
			if(right < ir_close_reading){
				// Turn right
				set_motor(1, v_go);
				set_motor(2, v_go*0.8);
			}
			else{
				// Turn left
				set_motor(1, v_go*0.8);
				set_motor(2, v_go);
			}
	}

	wallMazeStraight(right,left);
	wallMazeGate(right,left);
}

// Takes a left IR reading, and a right IR reading. Will calculate an error and drive the robot in the middle of the two walls
void wallMazeStraight(int right, int left){
	int dv = wallMazeOffset(right, left);
	if(dev){
		//fprintf(file, "v_go: %d  dv: %d\n", v_go, dv);
	}
	set_motor(1,v_go+dv);
	set_motor(2,v_go-dv);
}

// Handles the timed gate
void wallMazeGate(int right, int left){
	if(/*robot scans second red line*/){
		set_motor(1,0);
		set_motor(2,0);
		boolean gate_open = false;
		boolean gate_closed = false;
		int forward = read_analogue(mid_ir);
		//int gateDistance = ;   will need measurements
		//int gateOpenDistance = ;   will need measurements
		
		//gate is initially open
		// if gate open wait till closed and open again 
		if(forward > /*gateOpenDistance*/){ // if gate open
		
			while(gate_closed == false){ //keep ckecking if it closed
				if(forward < /*gateDistance*/){ 
					gate_closed = true; // gate is closed
				}
			}
			
			while(gate_open == false){ //keep ckecking if it reopens
				if(forward > /*gateOpenDistance*/){ 
					gate_open = true;
				}
			wallMazeStraight(right,left); // move through the gate
			}
		}
	
		//gate is initially closed
		//if gate closed wait till open
	
		if(forward < gateDistance){ // if gate closed
			
			while(gate_open == false){ // keep checking if it opens
				if(forward > /*gateOpenDistance*/){
					gate_open = true;
				}
			}
		wallMazeStraight(right,left); // move through the gate
		}	
	}
}

// Calculates an error value for getting the robot to drive in between the two walls
int wallMazeOffset(int right, int left){
 int error = (left-right);
 return ((error*maze_kp)+(error*maze_ki)*(error*maze_kd));
}

// Method that takes a row to scan and returns if there is a correct amount of red pixels to class as red tape. Requires that take_picture has been called;
bool scanRed(int scan_row){
	// Initialize the variable to store number of Reds in
	int numberReds = 0;
	//error value to ignore close values (to make sure that it is deffinitly red)
	int colourError = 30;
	//a threshold amount of red pixels it must pass to return true
	int redThreshold = 50;
	//go through all pixels. if below threshold its not a white pixel. if above then it is white
	for(int i = 100; i <220;i++){
		int pixRed = get_pixel(scan_row,i,0);
		int pixGreen = get_pixel(scan_row,i,1);
		int pixBlue = get_pixel(scan_row,i,2);
		//int pixWhite = get_pixel(scan_row,i,3;

		if((pixRed-colourError > pixGreen) && (pixRed-colourError > pixBlue)){
			// Increment the number of red pixels
			numberReds += 1;
		}
	}

	return (numberReds>redThreshold);
}
