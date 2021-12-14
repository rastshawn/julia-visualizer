#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>
#include <chrono>
#include <thread>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/def.h>
#include "complex.h"

/**
 * This program connects to a pulseaudio stream and uses volume levels in the left channel of that stream to 
 * generate Julia sets in a terminal. The volume level is set to the real component of the complex number, and 
 * the imaginary component is set to an oscilating value.
 * 
 * The audio capture is in a separate thread for performance. When the sleep function is removed from the drawing (main) thread, stuttering occurs. 
 */

// define colors for printing to the terminal
const std::string char_ = "\u2588";
const std::string black = "\033[22;30m";
const std::string red = "\033[22;31m";
const std::string l_red = "\033[01;31m";
const std::string green = "\033[22;32m";
const std::string l_green = "\033[01;32m";
const std::string orange = "\033[22;33m";
const std::string yellow = "\033[01;33m";
const std::string blue = "\033[22;34m";
const std::string l_blue = "\033[01;34m";
const std::string magenta = "\033[22;35m";
const std::string l_magenta = "\033[01;35m";
const std::string cyan = "\033[22;36m";
const std::string l_cyan = "\033[01;36m";
const std::string gray = "\033[22;37m";
const std::string white = "\033[01;37m";


// JULIA functions and variables //

// The edges of the screen will be mapped to these values on the julia set graph. 
const float xAxisMin = -1.5;
const float xAxisMax = 1.5;
const float yAxisMin = -1;
const float yAxisMax = 1;

// the shared seed complex number that will be frequently overwritten with audio information
Complex c = Complex(0, 0);

// this value is the loop count at which the intEscape function stops iterating
const int BAILOUT = 20;

// used in the intEscape function for figuring out the julia sets
Complex f(Complex x) {
	return x.multiply(x).add(c);
}

int intEscape(Complex c) {
	float num = 2;
	int loopCount = 0;
	while (c.lessThan(num) && loopCount < BAILOUT) {
		c = f(c);
		loopCount++;
	}

	if (!c.lessThan(num)) {
		return loopCount;
	} else return -1;
};

float interpolate(float var, float min1, float max1, float min2, float max2) {
	float numerator = (var - min1) * (max2 - min2);
	float denominator = (max1 - min1);
	float ret =  min2 + (numerator / denominator);
	return ret;
};


/*
 * Calculate the value of the julia function at this point.
 * @i the int value corresponding to the column pixel
 * @j the int value corresponding to the row pixel
 */
float julia (float i, float j, int width, int height) {

	float real = interpolate(i, 0, width-1, xAxisMin, xAxisMax);
	float imaginary = interpolate(j, 0, height-1, yAxisMin, yAxisMax);
	Complex complexSeed = Complex(real, imaginary);
	int esc = intEscape(complexSeed);
	if (esc == -1) {
		return 1; // black
	} else {
		return interpolate(esc, 0, BAILOUT, 0, 1);
	}
}


/*
 * Generates a string with all of the color values for the julia set 
 * on a screen of size width*height.
 * It then writes that string to the console. 
*/
void drawJulia(int width, int height) {
	std::stringstream frame;
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			
			// float x = x_start + j*dx; // current real value
			// float y = y_fin - i*dy; // current imaginary value
			
			float value = 100*julia(i, j, width, height);
			
			if (value == 100) {frame << " ";}
			else if (value > 90) {frame << red << char_;}
			else if (value > 80) {frame << l_red << char_;}
			else if (value > 70) {frame << orange << char_;}
			else if (value > 60) {frame << yellow << char_;}
			else if (value > 50) {frame << l_green << char_;}
			else if (value > 40) {frame << green << char_;}
			else if (value > 30) {frame << l_cyan << char_;}
			else if (value > 20) {frame << cyan << char_;}
			else if (value > 15) {frame << l_blue << char_;}
			else if (value > 10) {frame << blue << char_;}
			else if (value > 5) {frame << magenta << char_;}
			else {frame << l_magenta << char_;}

			
			frame << "\033[0m";
		}
		frame << std::endl;
	}
	frame << "\033[2J\033[1;1H"; // this clears the screen
	std::cout << frame.str();
}
// END JULIA FUNCTIONS //



// PulseAudio functions and variables //

// currentlyDisplayed holds the current volume level the user is seeing.
// This is done to allow for gradual level decay (prevents choppiness).
int currentlyDisplayedLeft = 0;
//int currentlyDisplayedRight = 0;
void setDisplayLevel() {
	pa_simple *s;
    pa_sample_spec ss;
    pa_buffer_attr pb;
	// I don't know what either of these values should be but they work
	// shamelessly copied from cava:
	// https://github.com/karlstav/cava
    pb.maxlength = (uint32_t) - 1;
    pb.fragsize = 1024 / 8 * 2 * 2;

    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 2;
    ss.rate = 44100;
    int error;
    s = pa_simple_new(NULL,               // Use the default server.
                    "Julia Visualizer",           // Our application's name.
                    PA_STREAM_RECORD,
                    NULL,               // Use the default device.
                    "Music",            // Description of our stream.
                    &ss,                // Our sample format.
                    NULL,               // Use default channel map
                    &pb,               // Use default buffering attributes.
                    &error
                    );
	
    int decayByL = 5;
    int decayByR = 5;

	// capture the audio frames, determine volume and decay, set the volume level to display
	for(;;) {

        int frames = 2048;
        short buf[frames];
        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0){
            std::cout << "Error reading " << error;
            break;
        }
		
		// skip every two, as buf[0] is left but buf[1] is right channel for same time
        for (int i = 0; i<frames; i+=2) {
            // left
            int measuredLeft = sqrt(pow(buf[i], 2));
            if (currentlyDisplayedLeft > measuredLeft) {
                currentlyDisplayedLeft -= decayByL;

            } else {
                //decayByL = 1;
                currentlyDisplayedLeft = measuredLeft;
            }
            // // right
            // int measuredRight = sqrt(pow(buf[i+1], 2));
            // if (currentlyDisplayedRight > measuredRight) {
            //     currentlyDisplayedRight -= decayByR;
            // } else {
            //     currentlyDisplayedRight = measuredRight;
            // }

            if (currentlyDisplayedLeft < 0) {
                currentlyDisplayedLeft = 0;
            }
        }
    
    }
}
// end pulseaudio functions //

int main() {

	// get the window size to set the width and height params
    struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	const int width = w.ws_col; //number of characters fitting horizontally on my screen 
	const int height = w.ws_row; //number of characters fitting vertically on my screen
    
	// Put the audio capture in its own thread
	std::thread getAudioSamples(setDisplayLevel);

	// the rate at which the imaginary component oscilates
	float di = -.005;
	// the max (and -min) of the imaginary component oscilation
	const float imaginaryRange = 1;

    for(;;) {

		// Map the display audio level to something that looks interesting in the julia map
		float real = interpolate(currentlyDisplayedLeft, 0, 16383, -.85, -.5);
		float imaginary = c.imaginary+di;

		if (imaginary > imaginaryRange || imaginary < -imaginaryRange) {
			di = -di;
		}

		// This is necessary to slow down the main thread. Otherwise it introduces graphical glitches
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));

		// change the Julia seed to reflect the calculated audio values
		c = Complex(real, imaginary);

		// calcluate and draw the julia set
		drawJulia(width, height);
    }

	// reconnect the sampling thread
	getAudioSamples.join();
}
