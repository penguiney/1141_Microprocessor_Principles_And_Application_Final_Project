#include <xc.h>

// PIC18F4520 Configuration Bit Settings
#pragma config OSC = INTIO67
#pragma config PWRT = OFF
#pragma config BOREN = ON
#pragma config WDT = OFF
#pragma config PBADEN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF

#define _XTAL_FREQ 1000000UL  // Fosc = 1MHz

volatile unsigned int time_count = 0;
volatile unsigned int index_speed = 2;// 1.5ms (0°)
unsigned int right_wheel[5] = {70, 70, 95, 110, 110};
unsigned int left_wheel[5] = {107, 107, 96, 70, 70};

//180° servo motor control
volatile int direction_ticks = 375; //Fosc = 1MHz, 1 tick = 4us, 375tick = 1500us = 0
volatile int atomic_direction_ticks = 375;
unsigned char phase = 0; //will set duty cycle high


void PWM_init(){
    //OSCCONbits.IRCF = 0b001; // 500 kHz
    T2CONbits.T2CKPS = 0b11;    // 16
    T2CONbits.TMR2ON = 1;
    PR2 = 255;            // PWM frequency ? 50 Hz

    CCP1CON = 0b00001100; // PWM mode
    TRISCbits.TRISC2 = 0; // RC2 = CCP1 output
    
    CCP2CON = 0b00001100;
    TRISCbits.TRISC1 = 0; // RC1 = CCP2 output
    index_speed = 2;
}

void PWM_set_duty(unsigned char duty1, unsigned char duty2){
    //if (duty > 624) duty = 624;

    CCPR1L = duty1 >> 2;          // 8 bits
    CCP1CONbits.DC1B = duty1 & 0x03; // 2 bits

    CCPR2L = duty2 >> 2;
    CCP2CONbits.DC2B = duty2 & 0x03; // 2 bits
}

void Timer3_Init(){
    T3CONbits.TMR3CS = 0;      // Timer3 clock = Fosc/4 = 1 MHz
    T3CONbits.T3CKPS = 0b00;   // Prescaler = 1
    T3CONbits.RD16 = 1;        // 16-bit mode
    
    TMR3 = 65536 - 5000;        //wait 20ms, a whole cycle

    PIR2bits.TMR3IF = 0;
    PIE2bits.TMR3IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    RCONbits.IPEN = 1;      // enable priority levels
    IPR2bits.TMR3IP = 1;    // Timer3 high priority

    TRISAbits.TRISA0 = 0;   // RA0 = software PWM output
    LATAbits.LATA0 = 0;
    T3CONbits.TMR3ON = 0;
}

void car_direction(int direction) {   
    //direction_ticks : servo motor accept 250 to 500, while the car accept 300 to 450
    if(direction == 1) direction_ticks = 420;
    if(direction == 0) direction_ticks = 375;
    if(direction == -1) direction_ticks = 330;
}

void car_speed(int speed) {
     if(speed == 1){ // forward
        // clockwise (~2.0 ms)
        PWM_set_duty(70,107);
        PWM_set_duty(90,100);
//        if(index_speed > 0) index_speed--;
//        PWM_set_duty(right_wheel[index_speed], left_wheel[index_speed]);
    }
    else if(speed == 0){ //stop
        // stop (~1.5 ms)
        PWM_set_duty(95,96);
//        if(index_speed < 2) index_speed++;
//        else if(index_speed > 2) index_speed--;
//        PWM_set_duty(right_wheel[index_speed], left_wheel[index_speed]);
    }
    else if(speed == -1){ //back
        // counterclockwise (~1.0 ms)
        PWM_set_duty(110,70);
        PWM_set_duty(100,85);
//        if(index_speed < 4) index_speed++;
//        PWM_set_duty(right_wheel[index_speed], left_wheel[index_speed]);
    }
}

void set_car_direction(int direction) {
    car_direction(direction);
}

void set_car_speed(int speed) {
    car_speed(speed);
}


//two times per 20ms
void __interrupt(high_priority) H_ISR(void) {
    if (PIR2bits.TMR3IF) {
        PIR2bits.TMR3IF = 0; // clean flag    

        if(phase == 0) { // duty cycle's high phase,  last for direction_ticks * 4 us
            atomic_direction_ticks = direction_ticks; //during a cycle, direction_ticks shouldnt be modify
            //if(atomic_direction_ticks > 500) atomic_direction_ticks = 500; //servo motor accept 500 as maximum(20000us)
            //TMR3 = 65536 - atomic_direction_ticks;
            switch(atomic_direction_ticks) {
                case 330:
                    TMR3 = 65536 - 330; break;
                case 420:
                    TMR3 = 65536 - 420; break;
                default:
                    TMR3 = 65536 - 375; break;
            }
//            if(atomic_direction_ticks == 300) TMR3 = 65536 - 300;
//            if(atomic_direction_ticks == 375) TMR3 = 65536 - 375;
//            if(atomic_direction_ticks == 450) TMR3 = 65536 - 450;
            phase = 1;
            LATAbits.LATA0 = 1; //turn RAO to high
            
        } else { // duty cycle's low phase,  last for 20000us - direction_ticks * 4 us
            //TMR3 = 60536 + atomic_direction_ticks;
            switch(atomic_direction_ticks) {
                case 330:
                    TMR3 = 60536 + 330; break;
                case 420:
                    TMR3 = 60536 + 420 ; break;
                default:
                    TMR3 = 60536 + 375; break;
            }
            phase = 0;
            LATAbits.LATA0 = 0; //turn RAO to low
        }
    }
}
