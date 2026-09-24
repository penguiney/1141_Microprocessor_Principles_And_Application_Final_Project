#include <xc.h>
#include <stdint.h>
#include <pic18f4520.h>

#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#define _XTAL_FREQ 1000000UL

uint8_t GetDirection(void);
unsigned char packet[50];
unsigned char count = 0;
unsigned char update_packet_flag = 1;
unsigned char celerate = 1; // decelerate 0 / hold 1 / accelerate 2
unsigned char direction = 1; // turn left 0 / hold 1 / turn right 2
unsigned char light = 0; // light on 0 / light off 1
unsigned char power = 0; // power on 0 / power off 1

void interrupt_init(void){
    RCONbits.IPEN = 0; //INT priority disable
    INTCONbits.GIE = 1; //General INT enable
	INTCONbits.INT0IF = 0; //Clear INT0 flag
    //INTCONbits.INT0IE = 1; //Enable INT0 INT
    INTCON2bits.INTEDG0 = 0; //Falling edge trigger for INT0
}

void __interrupt() isr(void) {
    if (PIR1bits.TMR1IF) {
        PIR1bits.TMR1IF = 0;
        TMR1 = 64786;
        LATDbits.LATD0 = packet[count];
        count++; 
        if(count > 49) count = 0;
        if(count == 1) update_packet_flag = 1;
    }
}

void timer1_init(void) {   
    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;
    T1CON = 0b00000001;   // Prescaler 1:1, Fosc/4, TMR1 ON
    TMR1 = 64786;
    PIR1bits.TMR1IF = 0;
    PIE1bits.TMR1IE = 1;
}

unsigned char sync_word[6] = {1, 0, 0, 0, 1, 0};
void packet_init() {
    unsigned char index = 0;
    for(int i = 0; i < 16; i++) packet[index++] = i % 2;
    packet[index++] = 1;
    for(int i = 0; i < 6; i++) packet[index++] = sync_word[i];
    for(int i = 0; i < 24; i++) packet[index++] = 0;
    while(index < 50) packet[index++] = (index % 5 == 0) ? 1 : 0;
}


void update_packet() {
    // @========================== packet (50 bit) =============================@
    // @  |   bit 0 ~ 16   | bit 17 ~ 22 |     bit 23 ~ 46    | bit 47 ~ 49 |   @
    // @  |    preamble    |    sync     |       payload      |     idle    |   @
    // @  |------51ms------|----18ms-----|--------72ms--------|-----9ms-----|   @
    // @  |------------------------------- 150ms ---------------------------|   @
    // @  timer1 send 1 bit every 3ms                                           @
    // @========================================================================@
    GetDirection();
    update_packet_flag = 0;
    //celerate:   slow down 00000000   / hold 00001111   /  speed up 11111111
    if (celerate == 2) for(int i = 23; i < 27; i++) packet[i] = 1;
    else for(int i = 23; i < 27; i++) packet[i] = 0;
    if (celerate == 0) for(int i = 27; i < 31; i++) packet[i] = 0;
    else for(int i = 27; i < 31; i++) packet[i] = 1;
    
    //direction:   turn left 00000000   / hold 00001111   /  turn right 11111111
    if (direction == 2) for(int i = 31; i < 35; i++) packet[i] = 1;
    else for(int i = 31; i < 35; i++) packet[i] = 0;
    if (direction == 0) for(int i = 35; i < 39; i++) packet[i] = 0;
    else for(int i = 35; i < 39; i++) packet[i] = 1;
     
    for(int i = 39; i < 43; i++) packet[i] = light; //light:   on 0000   / off 1111  
    for(int i = 43; i < 47; i++) packet[i] = power; //power:   on 0000   / off 1111  
}

uint16_t ADC_Read(uint8_t channel)
{
    ADCON0 &= 0xC3;
    ADCON0 |= (channel << 2);

    //__delay_ms(2);
    ADCON0bits.GO = 1;

    while (ADCON0bits.GO);

    return (ADRESH << 8) + ADRESL;
}

uint8_t GetDirection(void)
{
    uint16_t mid = 512;
    uint16_t th  = 150;   // threshold to avoid noise

    uint16_t vrx = ADC_Read(1);  // AN0
    uint16_t vry = ADC_Read(0);  // AN1
    uint8_t up    = (vry > mid + th);
    uint8_t down  = (vry < mid - th);
    uint8_t left  = (vrx < mid - th);
    uint8_t right = (vrx > mid + th);

    if(up == 1) celerate = 2;
    else if(down == 1) celerate = 0;
    else celerate = 1;
    
    
    if(left == 1) direction = 2;
    else if(right == 1) direction = 0;
    else direction = 1;
    if(left == 1)LATDbits.LATD1 = 1;
    else LATDbits.LATD1 = 0;
    if(right == 1)LATDbits.LATD2 = 1;
    else LATDbits.LATD2 = 0;
    return 0;
}

void ADC_Init(void)
{
    ADCON1 = 0x0E;   // AN0 AN1 analog
    ADCON2 = 0xA9;   // time seq setting
    ADCON0 = 0x01;   // ADC on, channel 0
}

void main(void) { 
    ADCON1 = 0x0E;   // AN0 AN1 analog
    OSCCON = 0b01000000; // 1MHz
    TRISD = 0;
    LATD = 0;
    timer1_init();
    
    packet_init();
    interrupt_init();
    ADC_Init();

    while(1) if(update_packet_flag == 1) update_packet(); 
}