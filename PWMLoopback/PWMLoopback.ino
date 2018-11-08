/*
* Brian R Taylor
* brian.taylor@bolderflight.com
* 
* Copyright (c) 2018 Bolder Flight Systems
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
* and associated documentation files (the "Software"), to deal in the Software without restriction, 
* including without limitation the rights to use, copy, modify, merge, publish, distribute, 
* sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is 
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or 
* substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
* BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
* Description: this program will output a sine wave on all 4 PWM channels for 
* testing the Teensy PWM Backpack (http://bolderflight.com/products/teensy/pwm). 
* The sine amplitude will be set to make the servo travel across its entire range
* and the excitation frequency will be 1 Hz.
*
* After these outputs are sent, they are looped back to 4 PWM channels by 
* connecting the PWM output pins to the PWM input pins. The PWM  commands 
* are read and printed to show that they are the same that were sent. This 
* is used to explore sending and reading PWM signals using the  
* Teensy PWM Backpack (http://bolderflight.com/products/teensy/pwm). 
*/

/* 
* Redefine the pin numbers to match the PWM Backpack numbering,
* we'll read the PWM input on 4 pins.
*/
const unsigned int PwmReadPins[4] = {5,6,10,9};
/*
* We'll write on the other 4 pins.
*/
const unsigned int PwmWritePins[4] = {23,22,21,20};
/*
* Measures duration with a microsecond level precision. We'll use
* this to measure the pulse width.
*/
elapsedMicros elapsedTime_us[4];
/*
* An array to store the measured pulse widths. Needs to be volatile
* so the compiler does not optimize it.
*/
volatile unsigned int PwmReadVal_us[4];
/*
* Storing the commanded pulse widths for comparison.
*/
unsigned int PwmWriteVal_us[4];
/* PWM update frequency, 50 Hz */
const unsigned int Freq = 50;
/* PWM update period */
const unsigned int Period_us = 1000000 / Freq;
/* PWM resolution */
const unsigned int PWM_Resolution = 16;
/* Time, ms */
elapsedMillis time_ms;
/*
* Our interrupt service routine (ISR) to measure the pulse width.
*/
void meas_pwm(unsigned int index) 
{
  /*
  * Read the value, if it's high, we just started the pulse and need
  * to zero the timer. If it's low, we just finished the pulse and
  * should store the pulse width result.
  */
  if (digitalReadFast(PwmReadPins[index])) {
    elapsedTime_us[index] = 0;
  } else {
    PwmReadVal_us[index] = elapsedTime_us[index];
  }
}
/*
* Call the ISR on a change in PWM 0 pin
*/
void meas_pwm0(void)
{
  meas_pwm(0); 
}
/*
* Call the ISR on a change in PWM 1 pin
*/
void meas_pwm1(void)
{
  meas_pwm(1); 
}
/*
* Call the ISR on a change in PWM 2 pin
*/
void meas_pwm2(void)
{
  meas_pwm(2); 
}
/*
* Call the ISR on a change in PWM 3 pin
*/
void meas_pwm3(void)
{
  meas_pwm(3); 
}

void setup()
{
  /*
  * Start USB serial to display results
  */
  Serial.begin(115200);
  while(!Serial) {}
 
  /* setting the analog frequency to 50 Hz and resolution to 16 bit */
  for (unsigned int i = 0; i < sizeof(PwmWritePins) / sizeof(unsigned int); i++) {
    analogWriteFrequency(PwmWritePins[i], Freq);
    analogWriteResolution(PWM_Resolution);
  }
 /*
  * Assign PWM pins as input
  */
  for (unsigned int i = 0; i < sizeof(PwmReadPins) / sizeof(unsigned int); i++) {
    pinMode(PwmReadPins[i],INPUT);
  }
  /*
  * Attach our ISRs to changes in the respective pins
  */
  attachInterrupt(PwmReadPins[0],meas_pwm0,CHANGE);
  attachInterrupt(PwmReadPins[1],meas_pwm1,CHANGE);
  attachInterrupt(PwmReadPins[2],meas_pwm2,CHANGE);
  attachInterrupt(PwmReadPins[3],meas_pwm3,CHANGE);
}

void loop()
{
  /*
  * Print the results
  */
  Serial.print(PwmReadVal_us[0]);
  Serial.print("\t");
  Serial.println(PwmWriteVal_us[0]);
  /* 1 Hz sine wave */
  float Cmd = sinf(2.0f * M_PI * time_ms / 1000.0f);
  /* Scale from +/- 1 to a range of 1000 us to 2000 us */
  Cmd = Cmd * 500.0f + 1500.0f;
  /* Command channels */
  for (unsigned int i = 0; i < sizeof(PwmWritePins) / sizeof(unsigned int); ++i) {
    PwmWriteVal_us[i] = Cmd;
    analogWrite(PwmWritePins[i],(float)PwmWriteVal_us[i] / Period_us * powf(2,PWM_Resolution));
  }
  delay(20);
}
