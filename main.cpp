#include <mbed.h>
#include "main.h"

// ECE 4180
// Georgia Institute of Technology
// This code is modified from the original code...
// for testing purposes with the fingerprint scanner removed

int main() {
    // arbitrary setup
    user = 0;
    volHit = 2;

    // Initialize FPS
    //sts = finger.Open();
    //if(sts ==0) {
    //    finger.CmosLed(1);
    //}
    
    // Initialize pc serial and interrupt
    pc.baud(115200);
    wait(1);
    pc.attach(&Serial_Recieved, Serial::RxIrq); // interrupt for pcSerial input
    modeSwitch.mode(PullUp);

    // check WDT timeout condition
    if ((LPC_WDT->WDMOD >> 2) & 1) {
        pc.printf("WDT timeout occurred\n\r");
    }
    // setup a 10 second timeout on watchdog timer hardware
    //wdt.kick(15.0);
    //pc.printf("going\n\r");
    playSound(playWAV[0]);

    while(1) {
        running = 1;
        switchChange();
        switch (buffin) {
            case 0x0000640D : // d, enter
                buffin = 0x00000000; // 4 byte reset
                default_test();
                break;
            case 0x0000700D : // p, enter
                buffin = 0x00000000; // 4 byte reset
                dpot_test();
                break;
            case 0x0000730D : // s, enter
                buffin = 0x00000000; // 4 byte reset
                wav_test();
                break;
            case 0x0000310D : // 1, enter
                buffin = 0x00000000; // 4 byte reset
                SonarDist1();
                break;
            case 0x0000320D : // 2, enter
                buffin = 0x00000000; // 4 byte reset
                SonarDist2();
                break;
            case 0x0000330D : // 3, enter
                buffin = 0x00000000; // 4 byte reset
                SonarDistBoth();
                break;
            case 0x0000670D : // g, enter
                buffin = 0x00000000; // 4 byte reset
                guiConnected = true;
                break;
        }
    }
}
void Serial_Recieved(void) {
    //pc.printf("Serial recieved\n");
    // pull in for hardware serial buffer and fill 3-byte software buffer
    if (pc.readable()) {
        holder = pc.getc(); // read bytes
        pc.putc(holder);
        buffin = buffin << 8;
        buffin = buffin | holder;
    }
    // if ENTER is hit, set "running" to 0 to exit operating mode
    if (holder == 0x0D) {
        running = 0;
        //pc.printf("\n\r");
    }
    // if ESC is hit, clear software buffer
    if (holder == 0x1B) {
        //pc.printf("clr\n\r");
        buffin = 0x00000000; // 4 byte reset
    }
}
void switchChange(void) {
    // set mbed mode (setup or play)
    if (!modeSwitch) {
        recLED = 1;
        myled4 = 0;
        SonarDistBoth();
    }
    else {
        recLED = 0;
        myled4 = 1;
        userSelect();
    }
}
void default_test(void) {
    //pc.printf("Default Test Mode\n");
    writeSPI(96);
    playSound(playWAV[0]);
    while(running) {
        hit1 = 1;
        hit2 = 0;
        wait(0.2);
        hit1 = 0;
        hit2 = 1;
        wait(0.2);
    }
    hit1 = 0;
    hit2 = 0;
}
void dpot_test(void) {
    // 7 bit - 0 to 128 
    //pc.printf("dpot_test\n");
    while(running) {
        //myled1 = 1;
        writeSPI(0);
        playSound(playWAV[0]);
        wait(1);
        //myled1 = 0;
        
        writeSPI(32);
        playSound(playWAV[0]);
        wait(1);
        
        writeSPI(64);
        playSound(playWAV[0]);
        wait(1);
        
        writeSPI(96);
        playSound(playWAV[0]);
        wait(1);
        
        writeSPI(128);
        playSound(playWAV[0]);
        wait(1);
    }
}
void wav_test(void) {
    //pc.printf("wav test\n");
    int k;
    writeSPI(0);
    while(running) {
        for(k=0;k<8;k++) {
            playSound(playWAV[k]);
            wait(1);
        }
    }
}
void SonarDist1(void) {
    drumReset(&drum1);
    drumReset(&drum2);
    
    totalTime.start();
    
    mu2.pauseUpdates();
    mu1.startUpdates();
    while(running) {
        if(modeSwitch) {
            running = 0;
        }
        mu1.checkDistance();
        // switchChange();
    }
    totalTime.stop();
    totalTime.reset();
}
void SonarDist2(void) {
    drumReset(&drum1);
    drumReset(&drum2);
    
    totalTime.start();
    
    mu1.pauseUpdates();
    mu2.startUpdates();
    while(running) {
        if(modeSwitch) {
            running = 0;
        }
        mu2.checkDistance();
        // switchChange();
    }
    totalTime.stop();
    totalTime.reset();
}
void SonarDistBoth(void) {
    drumReset(&drum1);
    drumReset(&drum2);
    
    totalTime.start();
    
    mu1.startUpdates();
    mu2.startUpdates();
    while(running) {
        if(modeSwitch) {
            running = 0;
        }
        mu1.checkDistance();
        mu2.checkDistance();
    }
    totalTime.stop();
    totalTime.reset();
}
void drumReset(Drum* drum) {
    drum->sampT.stop();
    drum->sampT.reset();
    drum->dT = 0; // dT between samples
    
    drum->distance = 0;
    drum->distanceHold1 = 0; // memory value for last raw distance
    drum->distanceHold = 0; // memory value for last raw distance
    drum->newD = 0; // filtered distance
    drum->lastD = 0; // distance memory value
    drum->startHitD = 0;
    drum->hitDD = 0;
    drum->sumVs = 0;
    
    drum->newV = 0; // filtered velocity
    drum->avgV = 0; // avg velocity
    drum->lastV = 0; // velocity memory value
    drum->v1 = 0; // velocity memory value
    drum->v2 = 0; // velocity memory value
    drum->v3 = 0; // velocity memory value
    
    drum->newA = 0; // filtered acceleration
    drum->avgA = 0;
    drum->a1 = 0; // acceleration memory value
    drum->a2 = 0; // acceleration memory value
}
void dist_update1(int distance_1) {
    drum1.distanceHold = (float)distance_1;
    dist_proc(&drum1, 0);
    hit_proc(&drum1, 0);
}
void dist_update2(int distance_2) {
    drum2.distanceHold = (float)distance_2;
    dist_proc(&drum2, 1);
    hit_proc(&drum2, 1);
}
void dist_proc(Drum* drum, int drumNum) {
    // find valid data - timing can mess up filter
    if (drum->sampT.read_us() > 1000000) {
        drum->sampT.stop();
        drum->sampT.reset();
    }
    
    if ( ((drum->distanceHold < 300) && (drum->distanceHold > 40)) && ((drum->distanceHold1 < 300) && (drum->distanceHold1 > 40))) {
        drum->distanceHold1 = drum->distanceHold;
        drum->distance = drum->distanceHold;
        drum->validD = 1;
    }
    else {
        drum->distanceHold1 = drum->distanceHold;
        drum->distance = drum->distance;
        drum->validD = 0;
        //return;
    }
    
    // determine change in time
    // determined in us: 1000000 us to s
    drum->sampT.stop();
    int holdDT = drum->sampT.read_us();
    drum->dT = (float)holdDT;
    drum->sampT.reset();
    drum->sampT.start();
    
    
    // determine change in distance
    // determined in mm: 1000 mm to m
    alphaD = drum->dT/(1000*250); // 1000 us per ms ; 1 hit width ~ 250 ms (higher = more smoothing)    
    drum->newD += alphaD * (drum->distance - drum->newD); // simple filter
    //drum->newDist = drum->distance; // direct
    drum->dD = drum->newD - drum->lastD;
    
    
    // determine velocity, with scaling and averaging
    if (drum->dT > 0) {
        drum->v = -(drum->dD*VGain)/drum->dT;
    }
    else {
        drum->v = 0;
    }
    alphaV = drum->dT/(1000*250); // 1000 us per ms ; 1 hit width ~ 250 ms (higher = more smoothing)
    //drum->newV += alphaV * (drum->v - drum->newV); // simple filter
    drum->newV = drum->v; // direct
    //drum->avgV = (drum->newV+drum->v1+drum->v2+drum->v3)/4; // average
    drum->avgV = drum->newV; // direct
    drum->dV = drum->avgV - drum->lastV;
    
    
    // determine acceleration, with scaling
    if (drum->dT > 0) {
        drum->a = (drum->dV*AGain)/drum->dT;
    }
    else {
        drum->a = 0;
    }
    alphaA = drum->dT/(1000*250); // 1000 us per ms ; 1 hit width ~ 250 ms (higher = more smoothing)
    //drum->newA += alphaA * (drum->a - drum->newA); // simple filter
    drum->newA = drum->a; // direct
    //drum->avgA = (drum->newA+drum->a1+drum->a2+drum->a3)/4; // average
    drum->avgA = drum->newA; // direct
    
    
    // update values for memory
    drum->lastD = drum->newD;
    drum->lastV = drum->avgV;
    drum->a3 = drum->a2;
    drum->a2 = drum->a1;
    drum->a1 = drum->avgA;
    drum->v3 = drum->v2;
    drum->v2 = drum->v1;
    drum->v1 = drum->avgV;
}
void hit_proc(Drum* drum, int drumNum) {
    // find valid data
    if (drum->validD == 0) {
        return;
    }
    
    if (drum->avgV < -8 && drum->canHit == 0) {
        drum->canHit = 1;
        drum->countForHit = 0;
        drum->v2 = 0;
        drum->v1 = 0;
        drum->startHitD = drum->newD;
        drum->sumVs = drum->avgV;
        drum->hitT.start();
    }
    
    if (drum->hitT.read_ms() > 350) {
        drum->canHit = 0;
        drum->countForHit = 0;
        drum->sumVs = 0;
        drum->hitT.stop();
        drum->hitT.reset();
    }
    else if (drum->canHit != 1) {
        drum->countForHit = 0;
         drum->sumVs = 0;
        drum->hitT.stop();
        drum->hitT.reset();
    }
    else {
        drum->sumVs += drum->avgV;
        if (drum->newD < drum->startHitD) {
            drum->startHitD = drum->newD;
        }
    }
    
    if (drum->newD < 300 && drum->v2 < -3 && drum->avgV > 0.5 && drum->canHit == 1 && drum->hitT.read_ms() < 1000) {
        hitLEDs[drumNum] = 1;
        hit_action(drum, drumNum);
        hitLEDs[drumNum] = 0; 
        drum->wasHit = 1;
        drum->canHit = 0;
        drum->hitT.reset();
    }
    else {
        drum->wasHit = 0;
    }
    
    //pc.printf("%d; T: %d filtD: %f filtV: %f filtA: %f canHit: %d count: %d wasHit: %d dT: %d\r\n", drumNum, totalTime.read_ms(), drum->newD, drum->avgV, drum->avgA, drum->canHit, drum->countForHit, drum->wasHit, holdDT);
}
void hit_action(Drum* drum, int drumNum) {
    int timeHold;
    
    drum->hitT.stop();
    holdDT = drum->hitT.read_ms();
    drum->hitDD = drum->newD - drum->startHitD;
    if (holdDT < 230) {
        volHit = 3;
    }
    else {
        volHit = 1;
    }
    playVolSet(volHit); 
    int k = (2*user)+drumNum;
    if (guiConnected) {
        timeHold = totalTime.read_ms();
        drum->hitPacket[0] = user;
        drum->hitPacket[1] = drumNum+1;
        drum->hitPacket[2] = (((unsigned char)timeHold) & (0xFF0000)) >> 16;
        drum->hitPacket[3] = (((unsigned char)timeHold) & (0x00FF00)) >> 8;
        drum->hitPacket[4] = ((unsigned char)timeHold) & (0x0000FF);
        drum->hitPacket[5] = volHit+1;
        for(int i=0;i<6;i++) {
            pc.putc(drum->hitPacket[i]);
            //pc.printf("%x", drum->hitPacket[i]);
        }
            //pc.printf("%d\n\r", k);    
    }
    playSound(playWAV[k]);
}
void playSound(char* wav) {
    //open wav file
    FILE *wave_file;
    wave_file=fopen(wav,"r");
    if(wave_file != NULL) 
    {
        waver.play(wave_file);
        fclose(wave_file);
        return;
    }
    pc.printf("Could not open file for reading - %s\n\r", wav);
    return;
}
void writeSPI(int n) { // 0 >= n <= 128
    cs=1;
    cs=0;
    spi.write(0x00); // Address(volatile) 0000, write=00, data=00 = 0x00
    spi.write(n); // Data 
    cs=1;
}
void playVolSet(int n) {
    switch (n) {
        case 3:
            writeSPI(0); // loud
            break;
        case 2:
            writeSPI(32); // mid high
            break;
        case 1:
            writeSPI(64); // mid low
            break;
        case 0:
            writeSPI(96); // low
            break;
        default:
              writeSPI(128); // off
    }
}
void userSelect(void) {
    /*if (finger.IsPress() == 1) {
        myled1 = 1;
        while(finger.Capture(1) != 0) {
            // loop until finger capture is good
        }
        user = finger.Identify() - 11;
        if(user >= 0 && user <= 3) {
            myled3 = 0;
        } else {
            myled3 = 1;
        }
        myled1 = 0;
        finger.WaitPress(0);
        wait(1);
    } */

    // As the code loops through this function...
    // it does the equivalent of selecting a random...
    // number for user when switched to play mode
    switch(user) {
        case 0 :
            user = 1;
            break;
        case 1 :
            user = 2;
            break;
        case 2 :
            user = 3;
            break;
        case 3 :
            user = 0;
        default :
            user = 0;
            break;
    }
}