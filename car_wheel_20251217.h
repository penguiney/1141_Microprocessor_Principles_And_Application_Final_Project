#ifndef CAR_WHEEL_H
#define    CAR_WHEEL_H

#include <xc.h> // include processor files - each processor file is guarded.  

void PWM_init();
void PWM_set_duty(unsigned char, unsigned char);
void Timer3_Init();
void set_car_speed(int);
void set_car_direction(int);
#endif  