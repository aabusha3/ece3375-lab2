//pointers to point to hardware
volatile int* buttons = (int*)0xFF200050;	//4 push buttons KEY_BASE
volatile int* timer = (int*)0xFFFEC600;	//MPCORE_PRIV_TIMER
volatile int* switch_ptr = (int*)0xFF200040;//SW_BASE
volatile int* hex_ptr = (int*)0xFF200020;//HEX3_HEX0_BASE
volatile int* hex_ptr2 = (int*)0xFF200030;//HEX5_HEX4_BASE

volatile int* t_control = (int*)0xFFFEC608; //the control register for the timer
volatile int* t_interrupt = (int*)0xFFFEC60C; //the interrupt status register for the timer


int h1, h2, m1, m2, s1, s2 = 0;
int lapH1, lapH2, lapM1, lapM2, lapS1, lapS2 = 0;
int startToggle = 0;
int startLapToggle = 0;
unsigned char lookupTable[16] = { 0x3F, 0x06, 0x5B, 0x4f, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67 };

//void displayLapped(); //not sure why this is here

char displayValue(int val) {
    return lookupTable[val];
}
void displayTime() {
    *((char*)hex_ptr) = displayValue(h1);
    *((char*)hex_ptr + 1) = displayValue(h2);
    *((char*)hex_ptr + 2) = displayValue(s1);
    *((char*)hex_ptr + 3) = displayValue(s2);
    *((char*)hex_ptr2) = displayValue(m1);
    *((char*)hex_ptr2 + 1) = displayValue(m2);
}
void displayLapped() {
    *((char*)hex_ptr) = displayValue(lapH1);
    *((char*)hex_ptr + 1) = displayValue(lapH2);
    *((char*)hex_ptr + 2) = displayValue(lapS1);
    *((char*)hex_ptr + 3) = displayValue(lapS2);
    *((char*)hex_ptr2) = displayValue(lapM1);
    *((char*)hex_ptr2 + 1) = displayValue(lapM2);
}
void updateTimer(int time) {
    h1 += time;
    if (h1 > 9) {
        h1 = 0;
        h2++;
    }
    if (h2 > 9) {
        h2 = 0;
        s1++;
    }

    if (s1 > 9) {
        s1 = 0;
        s2++;
    }
    if (s2 > 5) {
        s2 = 0;
        m1++;
    }
    if (m1 > 9) {
        m2++;
        m1 = 0;
    }
    if (m2 > 5) {
        m2 = 0;
        m1 = 0;
        s2 = 0;
        s1 = 0;
        h2 = 0;
        h1 = 1;
    }
}
void updateLapTimer(int time) {
    lapH1 += time;
    if (lapH1 > 9) {
        lapH1 = 0;
        lapH2++;
    }
    if (lapH2 > 9) {
        lapH2 = 0;
        lapS1++;
    }

    if (lapS1 > 9) {
        lapS1 = 0;
        lapS2++;
    }
    if (lapS2 > 5) {
        lapS2 = 0;
        lapM1++;
    }
    if (lapM1 > 9) {
        lapM1 = 0;
        lapM2++;
    }
    if (lapM2 > 5) {
        lapM2 = 0;
        lapM1 = 0;
        lapS2 = 0;
        lapS1 = 0;
        lapH2 = 0;
        lapH1 = 1;
    }
}
void clearTime() {
    h1 = 0;
    h2 = 0;
    m1 = 0;
    m2 = 0;
    s1 = 0;
    s2 = 0;
}
void clearLapTime() {
    lapS1 = 0;
    lapS2 = 0;
    lapM2 = 0;
    lapM1 = 0;
    lapH1 = 0;
    lapH2 = 0;
}

void storeLapTime() {
    lapH1 = h1;
    lapH2 = h2;
    lapM1 = m1;
    lapM2 = m2;
    lapS1 = s1;
    lapS2 = s2;
}

void setTimer() {
    //set the hardware timer load value to 0x001E8480 (2MHz)
    *(timer) = 0x001e8480;
}
void startTimer() {
    *(t_control) = 0x00000001;  //sets enable bit to 1, now we can start the timer
}

//this method is not in use because of the clock implementation
//my intended method is to have the private A9 continuoesly count down regardsless of what the 
//code or the FPGA are doing, then update the variables based on if the toggle is on or off  
//
/*void stopTimer() {
    *(t_control) = 0x00000000; //set enable bit of control register to 0
}*/

int checkTimer() {
    //check if the timer has counted all the way down
    //if it has, return 1 and set the timer back to the interval
    //else return 0

    if (*(t_interrupt) == 0b001) {   //check the 0 bit in interrupt status register
        *(t_interrupt) = 0b001;
        setTimer();
        return 1;
    }
    else {
        return 0;
    }

}

int readButtons(void) {
    volatile unsigned int buttonState = *buttons;
    buttonState = buttonState & 0xF;    //bit mask with 1111, determine state of the button
    return buttonState;
}

int readSwitches(void) {
    volatile unsigned int switchState = *switch_ptr;
    switchState = switchState & 0x1; //bit mask with 1, determine if switch 1 is on
    return switchState;
}

int main() {

    *(timer) = 0x001e8480;//set the hardware timer load value to 0x001E8480 (2MHz)

    while (1) {
        if (readButtons() == 0b0001) {//if button pressed, set condition to start timer and record a toggle
            if (readSwitches() != 1) {
                startToggle = 1;
            }
            else {
                startLapToggle = 1;
            }
        }
        startTimer();   //set enable bit

        if (*buttons == 0b0010) {    //if the stop button is pressed
            if (readSwitches() != 1) {
                startToggle = 0;
            }
            else {
                startLapToggle = 0;
            }
        }

        if (*buttons == 0b1000) {
            if (readSwitches() != 1) {
                clearTime();
            }
            else {
                clearLapTime();
            }
        }

        if (*buttons == 0b0100 && readSwitches() != 1) {
            storeLapTime();
        }

        if (checkTimer() == 1) { //if the timer has counted down from its value, add a ms to stopwatch
            if (startToggle == 1) {
                updateTimer(1);
            }
            if (startLapToggle == 1) {
                updateLapTimer(1);

            }

            if (readSwitches() != 1) {
                displayTime();
            }
            else {
                displayLapped();
            }

            setTimer();
        }
    }

    return 0;
}




