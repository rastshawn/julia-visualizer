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

float dist(float x1, float y1, float x2, float y2) {
	float result = sqrt(pow(x2-x1, 2) + pow(y2-y1, 2));
	return result;
}
class Complex {
	public: 
		float real;
		float imaginary;
		bool lessThan(float num) {
			return (dist(real, imaginary, 0, 0) < num);
		}
		Complex multiply(Complex x) {
			float newR = (this->real*x.real) - this->imaginary*x.imaginary;
			float newI = (this->real*x.imaginary) + this->imaginary*x.real;
			Complex ret(newR, newI);
			return ret;
		}
		Complex add(Complex x) {
			float newR = this->real + x.real;
			float newI = this->imaginary + x.imaginary;
			Complex ret(newR, newI);
			return ret;
		}
		Complex(float real, float imaginary) {
			this->real = real;
			this->imaginary = imaginary;
		}
};
int BAILOUT = 20;

float interpolate(float var, float min1, float max1, float min2, float max2) {
	float numerator = (var - min1) * (max2 - min2);
	float denominator = (max1 - min1);
	float ret =  min2 + (numerator / denominator);
	return ret;
};
//Complex c = Complex(-.66, -.71);
Complex c = Complex(0, 0);
Complex f(Complex x) {
	return x.multiply(x).add(c);
}

int intEscape(Complex c) {
	float num = 2;
	int loopCount = 0;
	while (c.lessThan(num) && loopCount < BAILOUT) {
		c = f(c);
		//std::cout << dist(c.imaginary, c.real, 0, 0) << std::endl;
		//std::cout << c.imaginary << " " << c.real << std::endl;
		loopCount++;
	}

	if (!c.lessThan(num)) {
		return loopCount;
	} else return -1;
};
float xx1 = -1.5;
float xx2 = 1.5;
float yx1 = -1;
float yx2 = 1;


float julia (float i, float j, int width, int height) {
	// notes, for audio we're really going to want to keep the real and imaginary components between -1 and 1
	// keep display between -1.5 and 1.5 or maybe 2 and -2
	float real = interpolate(i, 0, width-1, xx1, xx2);
	float imaginary = interpolate(j, 0, height-1, yx1, yx2);
	Complex complexSeed = Complex(real, imaginary);
	int esc = intEscape(complexSeed);
	if (esc == -1) {
		return 1; // black
	} else {
		return interpolate(esc, 0, BAILOUT, 0, 1);
	}
}

std::string char_ = "\u2588";

std::string black = "\033[22;30m";
std::string red = "\033[22;31m";
std::string l_red = "\033[01;31m";
std::string green = "\033[22;32m";
std::string l_green = "\033[01;32m";
std::string orange = "\033[22;33m";
std::string yellow = "\033[01;33m";
std::string blue = "\033[22;34m";
std::string l_blue = "\033[01;34m";
std::string magenta = "\033[22;35m";
std::string l_magenta = "\033[01;35m";
std::string cyan = "\033[22;36m";
std::string l_cyan = "\033[01;36m";
std::string gray = "\033[22;37m";
std::string white = "\033[01;37m";

void drawJulia(int width, int heigth) {


	std::stringstream frame;
	for (int j = 0; j < heigth; j++) {
		for (int i = 0; i < width; i++) {
			
			// float x = x_start + j*dx; // current real value
			// float y = y_fin - i*dy; // current imaginary value
			
			float value = 100*julia(i, j, width, heigth);
			
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
	frame << "\033[2J\033[1;1H";
	std::cout << frame.str();
}
int currentlyDisplayedLeft = 0;
int currentlyDisplayedRight = 0;
void setDisplayLevel() {
	pa_simple *s;
    pa_sample_spec ss;
    pa_buffer_attr pb;
    pb.maxlength = (uint32_t) - 1;
    pb.fragsize = 1024 / 8 * 2 * 2;


    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 2;
    ss.rate = 44100;
    int error;
    s = pa_simple_new(NULL,               // Use the default server.
                    "Fooapp",           // Our application's name.
                    PA_STREAM_RECORD,
                    NULL,               // Use the default device.
                    "Music",            // Description of our stream.
                    &ss,                // Our sample format.
                    NULL,               // Use default channel map
                    NULL,               // Use default buffering attributes.
                    &error
                    );
	

    int decayByL = 5;
    int decayByR = 5;

	for(;;) {

        int frames = 2048;
        short buf[frames];
        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0){
            std::cout << "Error reading " << error;
            break;
        }
		
        //std::cout << buf[0] << std::endl;
        for (int i = 0; i<frames; i+=2) {
            // left
            //buf[i] = sqrt(pow(buf[i], 2));
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


int main() {
    struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	//testInterpolate();
	//return 0;
	int width = w.ws_col; //number of characters fitting horizontally on my screen 
	int heigth = w.ws_row; //number of characters fitting vertically on my screen
    
	std::thread getAudioSamples(setDisplayLevel);

	float di = -.005;

    int skip = 256;
    int frameCount = 1;
    for(;;) {

		float real = interpolate(currentlyDisplayedLeft, 0, 16383, -.85, -.5);
		//std::cout << real << std::endl;
		float imaginary = c.imaginary+di;
		if (imaginary > 1 || imaginary < -1) {
			di = -di;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
		c = Complex(real, imaginary);
		// std::thread first (drawJulia, width, heigth);
		// first.join();
		drawJulia(width, heigth);

    
    }

	getAudioSamples.join();
}
