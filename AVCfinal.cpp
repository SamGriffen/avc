#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "E101.h"
// pointers to log files
FILE *file;
FILE *csv_log;

//********Universal variables (for all quadrants)
unsigned char v_go = 45; // average speed
int stage = 0; // what stage the bot is in 
bool dev = true; //log to text file?
bool csv_dev = true; //log to csv spreadsheet?
struct timeval start_time; //start time
struct timeval current_time; //time of current scan
struct timeval last_time; //time of previous scan
int whiteArray[320]; //array for pixel in a line
int total_error = 0; //total error
int prevError = 0; //prevError
int allWhiteCount = 200; //threshold of number of whitepixels in a row make a full white row

//********initiate methods so code compiles with main at top. (these are in chronological order)
//open gate at start
void openGate();


//***Following curved line
void curveyLineHandler(int scan_row);
int scanRow(int scan_row, int threshold);
int findCurveError();
void followLine(int error);

// Varialbes for PID control in curved line
double kp = 0.25;
double kd = 0.45;
double ki = 0.000;


//***Follow tape maze
void tapeMazeHandler(int scan_row);
int scanCol(int scan_col, int threshold);
void turnLeft();
void turnRight();
bool scanRed(int scan_row);

//motor correction for tape maze
int motorCorrection = 15;


//***Follow walled maze
void wallMazeHandler();
void wallMazeStraight(int right, int left);
int wallMazeOffset(int right, int left);
void mazeTurnLeft();
void mazeTurnRight();

// Declare pins for IR sensors
int left_ir = 0;
int mid_ir = 1;
int right_ir = 2;

// PID constants for maze
double maze_kp = 0.1;
double maze_kd = 0.2;
double maze_ki = 0.000;


int main(){
	init();

	// Open a file for logging text
	file = fopen("log.txt", "w");

	// Open a csv for logging to spreadsheet, and give headers to columns
	csv_log = fopen("csv_log.csv", "w");
	fprintf(csv_log, "Error, Time\n");

	// Define the row to scan on
	int scan_row = 120;
	
	//find time of day at program start, for derivative in PID control which needs change over time
	gettimeofday(&start_time, 0);

	//try to run the quadrants and turn off motors if program crashes
	try{
		//open the gate
		openGate();

		//run curved line till end
		while(stage == 0){
			curveyLineHandler(scan_row);
		}


		//run line maze till red tape marking end is seen
		while(stage == 1){
			fprintf(file, "THe 3rd quadrant has been reached\n");
			tapeMazeHandler(scan_row);
		}

		//run maze don't stop
		while(stage == 2){
			wallMazeHandler();
		}

	//if program crashes, turn off the motors
	}catch(long){
		set_motor(1,0);
		set_motor(2,0);}
	return 0;
}


//****************************open gate by recieveing and sending a password
void openGate(){
	//set up network variables controlling message sent/received and if connected or not
	char message[24] = {"Please"};
	char password[24] = {};
	int port = 1024;
	char serverAddress[15] = "130.195.6.196";

	//send and recieve the messages
	connect_to_server(serverAddress, port);
	send_to_server(message);
	receive_from_server(password);
	send_to_server(password);
}


//****************************Handels curvey line
void curveyLineHandler(int scan_row){
	// Take picture for this loop of logic
	take_picture();
	// Threshold for black/white differentiating
	int threshold = 120;

	// Populate the white array, and get the number of white pixels
	int numberWhites = scanRow(scan_row, threshold);
	
	//find new error and log with number of white pixels on line
	int error = findCurveError();
	if(dev){
		fprintf(file, "Error: %d  NOW: %d  Threshold: %d\n",error, numberWhites, threshold);;
	}

	//if all pixels black, back up the vehicle
	if(numberWhites < 10){
		set_motor(1, -1 * v_go);
		set_motor(2, -1 * v_go);
		fprintf(file, " line was all black, linewhiteness/threshold: %d\n", threshold);
	}
	//if all pixels are white, move onto the next stage
	else if(numberWhites > allWhiteCount){
		stage ++;
		fprintf(file, "Moved to the line maze\n");
	}
	//if mix of pixels, keep following the line
	else{
		followLine(error);
	}

}


//****************************Method that takes a row to scan, and a threshold value to work with, and returns the numer of white pixels in that scan row. Also populates the whiteArray
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


//****************************find direction error when following line/curve
int findCurveError(){
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


//****************************Follow lines that aren't perpendicular
void followLine(int error){
	//Add error from previous scan to total error. Find and use the time of day with the total error for calculating differential control
	total_error += error;
	gettimeofday(&current_time, 0);
	last_time = current_time;
	int errorDifference = (error - prevError)/( (current_time.tv_sec - last_time.tv_sec)*1000000+(current_time.tv_usec-last_time.tv_usec));

	//find the change from average speed needed to stay centered on line. Use PID and log
	int dv = (double)error * kp + (double)errorDifference * kd + (double)total_error * ki; 
	if(dev){
		fprintf(file, "P Action: %d  I Action: %d  D Action: %d\n", (int)((double)error * kp),(int)((double)total_error * ki),(int)((double)errorDifference * kd));
		
	}
	
	//Find the speed needed for the left and right wheels. Log values
	int v_left = v_go + dv;
	int v_right = v_go - dv;
	fprintf(file, "Left: %d Right: %d\n\n", v_left, v_right);

	// Get the current time and log
	if(csv_dev){
	 	long csv_time = (current_time.tv_sec - start_time.tv_sec) * 1000 + (current_time.tv_usec - start_time.tv_usec) / 1000;
		fprintf(csv_log, "%d,%ld\n", error, csv_time);
	}

	//set the motors to rotate with the required speed to follow line
	set_motor(1, v_left);
	set_motor(2, v_right);

	// Store the current error as the previous error for PID control (note prevError is a field)
	prevError = error; 
}


//****************************Handles tape maze
void tapeMazeHandler(int scan_row){
	// Take a new picture
	take_picture();
	
	//set average speed
	v_go = 45;
	
	// Thresholds for what make a pixel white and number of whitepixels in a line
	int minPix = 10;
	int threshold = 120;

	// Define our scan lines
	int leftCol = 40;
	int rightCol = 280;
	int midRow = 200;

	// Scan left, right, and center. Then log white pixels found in each scan
	int leftWhites = scanCol(leftCol, threshold);
	int rightWhites = scanCol(rightCol, threshold);
	int midWhites = scanRow(midRow, threshold);
	fprintf(file, "Left: %d Right: %d Mid: %d ", leftWhites, rightWhites, midWhites);

	// We want to prioritise a left turn, as left hand to the wall
	if(leftWhites > minPix && midWhites < minPix){
		turnLeft();
	}
	// If can't go left see if line ahead to follow
	else if(midWhites > minPix){
		scanRow(scan_row, threshold);
		int error = findCurveError();
		
		//follow the line at a lower speed to increase turning accuracy
		v_go = 40;
		followLine(error);
		v_go = 45;
	}
	// if no line left or straight, turn right
	else{
		turnRight();
	}

	// Check if we are looking at a red line, if so, go to the next quadrant
	if(scanRed(scan_row)){
		stage++;
		fprintf(file, "Moving into walled maze\n");
	}
}


//****************************Method that takes a column to scan, and a threshold value to work with, and returns the numer of white pixels in that column row. Also populates the whiteArray
int scanCol(int scan_col, int threshold){
	// Initialize the variable to store number of whites in
	int numberWhites = 0;
	
	//go through all pixels. if below threshold its not a white pixel. if above then it is white
	for(int i = 0; i <240;i++){
		int pix = get_pixel(i,scan_col,3);
		
		//if pixel more white than threshold, add One to array and add to count. else add Zero and not to count
		if(pix > threshold){
			numberWhites += 1;
			whiteArray[i] = 1;
		}
		else{
			whiteArray[i] = 0;
		}
	}
	return numberWhites;
}


//****************************Turn bot left until line to follow is found
void turnLeft(){
	int row = 20;
	int threshold = 120;
	
	//while less than 20 white pixels are seen, turn left
	int mid = scanRow(row, threshold);
	while(mid < 20){
		take_picture();
		mid = scanRow(row, threshold);
		fprintf(file, "Left: %d\n", mid);
		set_motor(1, 0);
		set_motor(2, v_go + motorCorrection);
	}
}


//****************************Turn bot right until the line to follow is found
void turnRight(){
	int row = 20;
	int threshold = 120;
	
	//while less than 20 white pixels are seen, turn right
	int mid = scanRow(row, threshold);
	while(mid < 20){
		take_picture();
		mid = scanRow(row, threshold);
		fprintf(file, "Right: %d\n", mid);
		set_motor(1, v_go + motorCorrection);
		set_motor(2, 0);
	}
}


//****************************Method that takes a row to scan and returns if there is a correct amount of red pixels to class as red tape
bool scanRed(int scan_row){
	// Initialize the variable to store number of Reds in
	int numberReds = 0;
	
	//error value to ignore close values (to make sure that it is deffinitly red)
	int colourError = 30;
	
	//a threshold amount of red pixels it must pass to return true
	int redThreshold = 50;
	
	//go through all pixels. if below threshold its not a white pixel. if above then it is white
	for(int i = 0; i <320;i++){
		int pixRed = get_pixel(scan_row,i,0);
		int pixGreen = get_pixel(scan_row,i,1);
		int pixBlue = get_pixel(scan_row,i,2);
		
		if((pixRed-colourError > pixGreen) && (pixRed-colourError > pixBlue)){
			numberReds += 1;
		}
	}

	return (numberReds>redThreshold);
}


//****************************Handles walled maze
void wallMazeHandler(){
	
	//set average speed
	v_go = 40;
	
	//get readings from the IR sensors and log them
	int left = read_analog(left_ir);
	int right = read_analog(right_ir);
	int front = read_analog(mid_ir);
	fprintf(file, "Left: %d Right: %d Front: %d ",left, right, front);
	
	//threshold to tell if bot is close to wall
	int irFrontReading = 180;
	int irSideReading = 180;

	//***logic for turns. priority order: straight, right, left

	//go forwards until can't
	if(front < irFrontReading || left > 300 || right > 300){
		wallMazeStraight(right,left);
		fprintf(file, "FORWARD");

	//go right if room on right
	}else if(right < irSideReading){
		mazeTurnRight();
		fprintf(file, "RIGHT");

	//turn left if no room on right and room on left
	}else if(left < irSideReading){
		mazeTurnLeft();
		fprintf(file, "LEFT");
	}
	//else if it can't go straight or turn in either direction, stop the bot. This should never happen
	else{
		set_motor(1, 0);
		set_motor(2, 0);
		fprintf(file, "There is no room in front or to either side");
	}
	fprintf(file, "\n");
}


//****************************Change motor speed to stay in center of corridor after finding error
void wallMazeStraight(int right, int left){
	int dv = wallMazeOffset(right, left);
	if(dev){
		fprintf(file, "v_go: %d  dv: %d\n", v_go, dv);
	}
	set_motor(1,v_go+dv);
	set_motor(2,v_go-dv);
}


//****************************Find turn needed to stay in center of corridor, with PID
int wallMazeOffset(int right, int left){
	int error = (left-right);
	return ((error*maze_kp)+(error*maze_ki)*(error*maze_kd));
}


//****************************Change motor speed preportional to v_go to turn left or right
void mazeTurnLeft(){
	set_motor(1, v_go * 0.4);
	set_motor(2, v_go * 0.6);
}
void mazeTurnRight(){
	set_motor(1, v_go * 0.6);
	set_motor(2, v_go * 0.4);
}


