#include <xc.h>
#include "car_wheel.h"
#include <pic18f4520.h>
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#define _XTAL_FREQ 1000000UL

int count = 0;
unsigned char ms_flag = 0;
void __interrupt(low_priority) L_ISR(void) {
    if (PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;
        TMR1 = 64786;
        ms_flag = 1;
    }
}

void timer1_init(void) {   
    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;
    RCONbits.IPEN = 1;      // enable priority levels
    T1CON = 0b00000001;
    TMR1 = 64786;
    PIR1bits.TMR1IF = 0;
    PIE1bits.TMR1IE = 1;
    IPR1bits.TMR1IP = 0;    // Timer1 low priority
}

void car_Init(){
    PWM_init();
    ADCON1 = 0x0F;
    TRISBbits.TRISB0 = 1;  // RB0 as input (button)
    //PWM_set_duty(46);
    //car_speed = 0;
    PWM_set_duty(95,96);
    T2CONbits.TMR2ON = 1;
}

void main(void)
{
    ADCON1 = 0x0F;
    OSCCON = 0b01000000; // 1MHz
    TRISB = 255;
    TRISCbits.TRISC7 = 0;
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC5 = 0;
    LATCbits.LATC7 = 0;
    LATCbits.LATC6 = 0;
    LATCbits.LATC5 = 0;
    TRISD = 0;
    TRISA = 0;
    LATD = 255;
    timer1_init();
    car_Init();
    Timer3_Init();
    
    unsigned char recv_bit = 0;
    unsigned char state = 0;
    unsigned char preamble = 0;
    unsigned char sync_word[6] = {1, 0, 0, 0, 1, 0};
    unsigned char sync = 0;
    unsigned char recv = 0;

    unsigned char tmp_celerate, tmp_direction, tmp_light, tmp_power = 0;
    unsigned char celerate, direction, light, power = 0;

    while(1) {    
        if(ms_flag == 1) { 
            ms_flag = 0; //clean 3ms flag                     
            recv_bit = PORTBbits.RB0;
            
            //FSM
            if(state == 0) { //preamable
                if(recv_bit == preamble % 2) preamble++;
                else if(preamble > 6) {
                    state++;
                    LATCbits.LATC6 ^= 1;
                } else preamble = 0;             
            } else if(state == 1) { //sync word
                if(recv_bit == sync_word[sync]) sync++;
                else {
                    state = 0;
                    preamble = 0;
                    sync = 0;
                }
                if(sync == 6) {
                    state++;
                    recv = 0;
                    tmp_celerate = 0;
                    tmp_direction = 0;
                    tmp_light = 0;
                    tmp_power = 0;
                    LATCbits.LATC7 ^= 1;
                }
            } else if(state == 2) { //receive data      
                if(recv_bit == 1) {
                    if(recv < 8) tmp_celerate++;
                    else if(recv < 16) tmp_direction++;
                    else if(recv < 24) tmp_light++;
                    else tmp_power++;
                }
                recv++;
                if(recv >= 32) {                  
                    state = 0;
                    preamble = 0;    
                    LATD = 0;
                     //celerate:   slow down 0-2 1's   / hold 3-5 1's   /  speed up 6-8 1's
                    if (tmp_celerate < 3) celerate = 0;
                    else if (tmp_celerate < 6) celerate = 1;
                    else celerate = 2;
                    if(celerate == 0) LATDbits.LATD0 = 1;
                    if(celerate == 1) LATDbits.LATD1 = 1;
                    if(celerate == 2) LATDbits.LATD2 = 1;
                    
                    //direction:   turn left 0-2 1's   / center 3-5 1's   /  turn right 6-8 1's
                    if (tmp_direction < 3) direction = 0;
                    else if (tmp_direction < 6) direction = 1;
                    else direction = 2;
                    direction = 2 - direction;
                    if(direction == 0) LATDbits.LATD6 = 1;
                    if(direction == 1) LATDbits.LATD5 = 1;
                    if(direction == 2) LATDbits.LATD4 = 1;
                    
                    
                    //not implemented
                    //light:   off 0-1 1's  / on 2-4 1's  
                    if (tmp_light <= 1) light = 0;
                    else light = 1;
                    if(light == 0) LATDbits.LATD3 = 1;
                    
                    //power:   off 0-1 1's  / on 2-4 1's  
                    if (tmp_power <= 1) power = 0;
                    else power = 1;
                    if(power == 0) LATDbits.LATD7 = 1;       
                    
                    set_car_direction(direction - 1);
                    set_car_speed(celerate - 1);                   
                }
            }  
        }
    }
}