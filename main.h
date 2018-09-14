#ifndef MAIN_H
#define MAIN_H
#include "ultrasonic.h"
#include "SDFileSystem.h"
#include "wave_player.h"
//#include "GT511C3.hpp"

// ECE 4180
// Georgia Institute of Technology
// This code is modified from the original code...
// for testing purposes with the fingerprint scanner removed

class Watchdog {
public:
    // Load timeout value in watchdog timer and enable
    void kick(float s) {
        LPC_WDT->WDCLKSEL = 0x1;                // Set CLK src to PCLK
        uint32_t clk = SystemCoreClock / 16;    // WD has a fixed /4 prescaler, PCLK default is /4
        LPC_WDT->WDTC = s * (float)clk;
        LPC_WDT->WDMOD = 0x3;                   // Enabled and Reset
        kick();
    }
    // "kick" or "feed" the dog - reset the watchdog timer
    // by writing this required bit pattern
    void kick() {
        LPC_WDT->WDFEED = 0xAA;
        LPC_WDT->WDFEED = 0x55;
    }
};


typedef struct drum {
    Timer sampT;
    Timer hitT;
    float distance, distanceHold, distanceHold1; // from sensor
    float dT, dD, dV; // delta values
    float lastD, lastV; // remembered values
    float newD, newV, newA; // filtered values
    float avgV, v, v1, v2, v3; // remembered v values
    float avgA, a, a1, a2, a3; // remembered accel values
    float startHitD, hitDD, sumVs;
    int validD, wasHit, canHit, countForHit;
    unsigned char hitPacket[6];
} Drum;

// function setup
void RxInt(void); // usb serial interrupt handler
void Serial_Recieved(void); // processes usb serial data
void switchChange(void); // processes switch 
void default_test(void); // plays a .wav and flashed LEDs
void dpot_test(void); // plays a .wav at different volume levels
void wav_test(void);
void SonarDist1(void); // operates drum 1 only
void SonarDist2(void); // operates drum 2 only
void SonarDistBoth(void); // operates both drums
void drumReset(Drum* drum); // resets all drum class variables
void dist_update1(int); // handler function to update drum 1 distance readings
void dist_update2(int); // handler function to update drum 2 distance readings
void dist_proc(Drum* drum, int); // process disance, velocity, and acceleration values
void hit_proc(Drum* drum, int); // determines if a drum hit has occurred
void hit_action(Drum* drum, int); // executes if a hit has occurred
void playSound(char* wav); // uses SD and .wav libraries
void writeSPI(int); // write to d pot
void playVolSet(int); // sets enumerated volume
void userSelect(void); // sets the global variable 

// inputs
Serial pc(USBTX, USBRX);
DigitalIn modeSwitch(p25);
ultrasonic mu1(p21, p22, .005, 1, &dist_update1);   //Set the trigger pin to p21 and the echo pin to p22
ultrasonic mu2(p24, p23, .005, 1, &dist_update2);   //Set the trigger pin to p24 and the echo pin to p23
SDFileSystem sd(p5, p6, p7, p8, "sd"); // the pinout on the mbed Cool Components workshop board
// GT511C3 finger(p28, p27); // Fingerprint scanner (_tx, _rx)

// outputs
DigitalOut myled1(LED1);
DigitalOut myled2(LED2);
DigitalOut myled3(LED3);
DigitalOut myled4(LED4);
DigitalOut hit1(p19);
DigitalOut hit2(p30);
DigitalOut hitLEDs[2] = {hit1, hit2};
DigitalOut recLED(p15);
AnalogOut DACout(p18);
SPI spi(p11, p12, p13); // mosi, miso, sclk. To write commands only, you do not need to connect p6(miso) to MCP4141.
DigitalOut cs(p14); // Chip select. Active low.


// globals
uint8_t volatile holder; // 1 byte
uint64_t volatile buffin; // 4 bytes
unsigned int volatile running = 0;
volatile bool guiConnected = false;
uint8_t volatile user;
Drum drum1;
Drum drum2;
int distance_1, distance_2; // from sensor
#define VGain 100000 // converts (mm/us) to 100x(m/s)
#define AGain 1000000 // converts 100x(m/s / us) to 100x(m/s /s)
// 1 hit ~250 ms
float alphaD;
float alphaV;
float alphaA;
wave_player waver(&DACout);
Timer totalTime;
uint8_t volHit;
int holdDT;
int sts;


// files for .wav playing
char play1[] = "/sd/wavfiles/DRUM1_trim.wav";
char play2[] = "/sd/wavfiles/DRUM2_trim.wav";
char play3[] = "/sd/wavfiles/DRUM3_trim.wav";
char play4[] = "/sd/wavfiles/DRUM4_trim.wav";
char play5[] = "/sd/wavfiles/DRUM5_trim.wav";
char play6[] = "/sd/wavfiles/DRUM6_trim.wav";
char play8[] = "/sd/wavfiles/DRUM8_trim.wav";
char play9[] = "/sd/wavfiles/DRUM9_trim.wav";
char *playWAV[8] = {play1, play2, play3, play4, play5, play6, play8, play9};


#endif