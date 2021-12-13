
#include <iostream>
#include <sstream>
#include <math.h>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/def.h>


int main() {
    std::cout << "Hello world";
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

    int currentlyDisplayedLeft = 0;
    int currentlyDisplayedRight = 0;
    int decayByL = 40;
    int decayByR = 5;

    int skip = 1024;
    int frameCount = 0;
    for(;;) {

        int frames = 1024;
        short buf[frames];
        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0){
            std::cout << "Error reading " << error;
            break;
        }
    
        //std::cout << buf[0] << std::endl;
        for (int i = 0; i<frames; i+=2) {


            //std::cout << "left: " << (short) buf[i] << " right: " << (short) (buf[i+1]) << std::endl;



            // left
            //buf[i] = sqrt(pow(buf[i], 2));
            int measuredLeft = 100*sqrt(pow(buf[i], 2));
            if (currentlyDisplayedLeft > measuredLeft) {
                currentlyDisplayedLeft -= decayByL;

            } else {
                //decayByL = 1;
                currentlyDisplayedLeft = measuredLeft;
            }
            
            // right
            buf[i+1] = (100*sqrt(pow(buf[i+1], 2))) / 16383;
            if (currentlyDisplayedRight > buf[i+1]) {
                currentlyDisplayedRight -= decayByR;
                decayByR *= 5;
            } else {
                decayByR = 5;
                currentlyDisplayedRight = buf[i+1];
            }

            //std::string channel = (i%2 == 0) ? "L: " : "R: ";
            if (currentlyDisplayedLeft < 0) {
                currentlyDisplayedLeft = 0;
            }
            //int outputL = currentlyDisplayedLeft;
            // std::cout << "L: " << currentlyDisplayedLeft << std::endl;
            if (i%512 == 0) {
                std::string outputL((currentlyDisplayedLeft) / 16383, '|');
                //std::string outputR(currentlyDisplayedLeft / 10, '|');
                std::cout << "L: " << outputL << std::endl;
            }


        }

        // for (int currentFrame = 0; currentFrame < frames; currentFrame++) {
        //     uint8_t measuredLevel = buf[currentFrame];
        //     // decay
        //     if (currentlyDisplayedLevel > measuredLevel) {
        //         currentlyDisplayedLevel -= decayBy;
        //         decayBy *= 5;
        //     } else {
        //         currentlyDisplayedLevel = measuredLevel;
        //         decayBy = 0;
        //     }

        //     if (currentFrame%skip == 0) {
        //         //std::cout << currentlyDisplayedLevel << std::endl;

        //         // int numDashes = currentlyDisplayedLevel / 600;
        //         // std::string output(numDashes, '-');
        //         // std::cout << output << std::endl;
                
        //         std::cout << measuredLevel << std::endl;
        //     }
        // }
    
    }
}
