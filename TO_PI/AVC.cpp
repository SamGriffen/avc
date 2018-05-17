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

//array holding average whiteness of a line
int averageWhiteArray[100];
int averageCounter = 0;
int whitenessOfScans = 0;
int normalWhiteness = 0;

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
int findCurveError(int scan_row, int threshold);
int followLine(int error, int scan_row, int threshold);
int followMaze();
void openGate();
void changeWhitenessArray(int scan_row);
void curveyLineHandler(int error, int scan_row);
void tapeMazeHandler(int error, int scan_row);

int main(){
	init();

	// Open a file for logging
	file = fopen("log.txt", "w");

	try{
		
		//find the average whiteness of a normal line
		int scan_row = 120;
		
		
		//int normalWhiteness =       old normal whiteness. now array average
		//takes 100 scans and puts into an array
		while(averageCounter < 100){
			averageWhiteArray[averageCounter] = findMinMax(scan_row);
			averageCounter ++;
		}
		averageCounter = 0;
		
		
		//goes through array of whiteness to find average
		while(averageCounter < 100){
			whitenessOfScans += averageWhiteArray[averageCounter];
			averageCounter ++;
		}
		normalWhiteness = whitenessOfScans/100;
		averageCounter = 0;
		
		
		//openGate();
		// sleep1(5,0);

		//run line
		int error = 0;
		while(stage == 0){
			curveyLineHandler(error, scan_row);
		}
	
	
		//run line maze
		while(stage == 1){
			//tapeMazeHandler(error, scan_row);
			fprintf(file, "THe 3rd quadrant has been reached\n");
			set_motor(1, 0);
			set_motor(2, 0);
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

void curveyLineHandler(int error, int scan_row){
	//change what the average whiteness of a line looks like
	changeWhitenessArray(scan_row);
	
	//find max and min values of whiteness on the scan line
	int threshold = findMinMax(scan_row);
	if(dev){
		fprintf(file, "CURRENT READING: %d\n", threshold);;
	}

	//if all pixels black, back up the vehicle
	if(threshold < normalWhiteness - lowerIntensityMargin){
		set_motor(1, -1 * v_go);
		set_motor(2, -1 * v_go);
		fprintf(file, " line was all black, linewhiteness/threshold: %d\n", threshold);

	//if all pixels are white, move onto the next stage
	}else if(threshold > normalWhiteness + upperIntensityMargin){
		stage ++;
		fprintf(file, " moved to next stage linewhiteness/threshold: %d\n", threshold);
		
	//if mix of pixels, keep following the line
	}else{
		error = followLine(error, scan_row, threshold);
	}
	//reset min and max
	min = 255;
	max = 0;
}

void tapeMazeHandler(int error, int scan_row){
	
	//change what the average whiteness of a line looks like
	changeWhitenessArray(scan_row);
	
	///take scan
	//find max and min values of whiteness on the scan line
	upperIndex = 320;
	lowerIndex = 0;
	//reset min and max
	min = 255;
	max = 0;
	int threshold = findMinMax(scan_row);
	if(dev){
		printf("threshold: %d\n", threshold);;
	}

	///if all white
		///move bot forwards
	if(threshold > normalWhiteness + upperIntensityMargin){
		set_motor(1, v_go);
		set_motor(2, v_go);
		if(dev){
			printf("lineWhiteness: %d, all white/threshold\n", threshold);
		}
	}

	///else if all black
		///turn bot hard left
	else if(threshold < normalWhiteness - lowerIntensityMargin){
		set_motor(1, 0);
		set_motor(2, v_go);
		if(dev){
			printf("lineWhiteness/threshold: %d\n", threshold);
		}
	
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
		threshold = findMinMax(scan_row);
		
		if(threshold > normalWhiteness + upperIntensityMargin){
			scan_row = 220;
			threshold = findMinMax(scan_row);
			if(threshold < normalWhiteness - lowerIntensityMargin){
				set_motor(1, v_go);
				set_motor(2, v_go);
			}else{
				set_motor(1, 0);
				set_motor(2, v_go);
			}
			scan_row = 120;
			
			if(dev){
				printf("lineWhiteness/threshold: %d , left all white\n", threshold);
			}
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
		threshold = findMinMax(scan_row);
		if(threshold > normalWhiteness/2 + upperIntensityMargin){
			scan_row = 220;
			min = 255;
			max = 0;
			threshold = findMinMax(scan_row);
			if(threshold < normalWhiteness - lowerIntensityMargin){
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
			error = followLine(error, scan_row, threshold);
			//reset min and max
			min = 255;
			max = 0;
			if(dev){
				printf("adjusting to line \n");
			}
		}
			
	}
	
}

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
