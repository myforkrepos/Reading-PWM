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
* Description: this program will read PWM values from an RC receiver
* using the Teensy PWM Backpack (http://bolderflight.com/products/teensy/pwm)
* and print the results. Interrupt service routines are used to
* measure the input PWM signals. 
*/

/* 
* Redefine the pin numbers to match the PWM Backpack numbering,
* we'll read the PWM input on 4 pins.
*/
const unsigned int PwmReadPins[4] = {5,6,10,9};
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
  Serial.print(PwmReadVal_us[1]);
  Serial.print("\t");
  Serial.print(PwmReadVal_us[2]);
  Serial.print("\t");
  Serial.println(PwmReadVal_us[3]);
  delay(20);
}
