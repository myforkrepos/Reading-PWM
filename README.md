# Reading PWM
In this tutorial we'll read PWM commands with our microcontroller. If you haven't already, you may consider going through the [PWM Introduction tutorial](https://github.com/bolderflight/PWM-Intro), which explains what PWM is and provides practical experience through commanding a servo using PWM.

For the initial part of this tutorial, we will loop our PWM transmitter pin back to our PWM receiver pin. This will allow us to: create a PWM signal and then receive it to make sure that it's the same that was sent. The second part of this tutorial will involve reading PWM signals from an RC receiver paired with a transmitter.

# Necessary Hardware
   * [Teensy 3.2](https://www.pjrc.com/store/teensy32.html)
   * [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/)
   * An RC transmitter and receiver will be used during part of this tutorial, but you'll still be able to complete most of it without these items.

# PWM Loopback
## Wiring
Connect the signal pins from opposite sides of the [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/). Signal pin 1 should be connected to pin 8, pin 2 to pin 7, and so on. We'll use half the [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/) for creating and sending the PWM signal and the other half for reading the PWM signal.

<img src="/images/loopback.JPG" alt="loopback" width="500">

Plug your [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) into the [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/) with the USB connector on the end of the backpack that has a white dot; this dot marks the location of the Teensy ground pin. 

<img src="/images/loopback_setup.JPG" alt="loopback setup" width="500">

## Software
We'll use the [Teensy PWM](https://www.pjrc.com/teensy/td_pulse.html) library for creating the PWM signals. We'll make our own methods for reading the PWM signals.

### Installation
For this tutorial, you can clone or download the code in this [repository](https://github.com/bolderflight/Reading-PWM) and follow along or code it yourself. We'll use the _PWMLoopback.ino_ for this first test.

### Tutorial
#### Goals
The goal for this initial test is to use the sine wave we programmed in the [PWM Introduction tutorial](https://github.com/bolderflight/PWM-Intro) to create a PWM signal on four of the [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/) pins. On the other four pins, we'll measure the pulse width and see if they match.

### Code Walkthrough
#### Globals
We need to add some global variables. First, we'll remap the Teensy pins to an array of pins to read and an array of pins to write to.

```C++
/* 
* Redefine the pin numbers to match the PWM Backpack numbering,
* we'll read the PWM input on 4 pins.
*/
const unsigned int PwmReadPins[4] = {5,6,10,9};
/*
* We'll write on the other 4 pins.
*/
const unsigned int PwmWritePins[4] = {23,22,21,20};
```

We'll also create an array of microsecond precision timers for measuring the pulse widths and arrays for storing the written pulse widths and read pulse widths.

```C++
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
```

The rest of the globals should look familiar from the [PWM Introduction tutorial](https://github.com/bolderflight/PWM-Intro), we'll need them for writing the PWM signals.

```C++
/* PWM update frequency, 50 Hz */
const unsigned int Freq = 50;
/* PWM update period */
const unsigned int Period_us = 1000000 / Freq;
/* PWM resolution */
const unsigned int PWM_Resolution = 16;
/* Time, ms */
elapsedMillis time_ms;
```

#### ISR
We're going to use interrupts for measuring the pulse widths. Specifically, we'll call an interrupt every time the state one of the PWM measurement pins changes (either high to low or low to high). Interrupt service routines should be kept short to allow the microcontroller to quickly leave its interrupted state. We're using interrupts here because they enable our code to be efficient, we don't need to sit in loop and continuously poll the pin state to find when it changes. This is especially important when reading several pins and also trying to create the PWM signals. Using interrupts also lets us measure PWM accurately by interrupting the code immediately when the pin state changes.

Here's the Interrupt Service Routine (ISR) that we'll use:

```C++
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
```

It's pretty simple - once we get an interrupt we'll read the pin state. If the pin is high, this means the pin transitioned from low to high and that we're at the start of a PWM pulse, so we zero our timer. If the pin is low, this means the pin transitioned from high to low and we're at the end of the PWM pulse, so we store the timer value in our array, the timer value at this point is the measurement of the pulse width.

We wrote our ISR with index as an input, so it will work for any of our PWM read pins. However, we don't get the interrupt context, so we'll wrap this function with an ISR for each pin.

```C++
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
```

#### Setup
Again, most of this should look familiar from the [PWM Introduction tutorial](https://github.com/bolderflight/PWM-Intro). We'll start the USB serial communications for displaying results and setup the frequency and resolution for creating our PWM signals.

```C++
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
```

Next, we'll set the pin modes and assign interrupts for all of our PWM measurement pins.

```C++
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
```

#### Loop
Here we're adding a printout of the created pulse width and measured pulse width for comparison. Otherwise, the rest of this function is nearly identical to the [PWM Introduction tutorial](https://github.com/bolderflight/PWM-Intro).

```C++
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
```

That's it! For a full code listing, please see _PWMLoopback.ino_.

## Experiment
Upload your code to the Teensy and open the Serial Monitor. You should a stream of two columns of values and notice that these values match or nearly match. This means we're successfully measuring the PWM signals that we've created!

<img src="/images/loopback_exp.png" alt="loopback exp" width="500">

## Wrap Up
In this initial experiment, we looped our PWM pins to each other and used four pins for creating PWM signals and four for measuring PWM signals. We validated our PWM measurement approach by checking that our measured PWM data matches our transmitted data.

# RC Transmitter and Receiver
## Wiring
For this part of the tutorial, we will be connecting an RC receiver to the [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/). We would like to power the RC receiver from the Teensy as well, so make sure the voltage solderpad is set to 5V instead of VDD (this sends 5V to the SBUS RX power pin instead of the bussed PWM servo power).

<img src="/images/rx.JPG" alt="receiver" width="500">

Bind an RC receiver to a transmitter and connect the receiver to the [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/). We'll use pins 5 - 8 for the PWM measurement, so connect the receiver power and ground to the power and ground pins on the pin labled SBUS RX. Connect the receiver signal pins to [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/) pins 5 - 8. Finally, plug your [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) into the [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/) with the USB connector on the end of the backpack that has a white dot; this dot marks the location of the Teensy ground pin.

<img src="/images/rx_setup.JPG" alt="receiver setup" width="500">

## Software
We'll make our own methods for reading the PWM signals, so no libraries are needed!

### Installation
For this tutorial, you can clone or download the code in this [repository](https://github.com/bolderflight/Reading-PWM) and follow along or code it yourself. We'll use the _PWMReceiver.ino_ for this test.

### Tutorial
#### Goals
The goal of this tutorial is to measure pulse widths from an RC receiver connected to a transmitter. We'll display the PWM data to view the stick positions.

### Code Walkthrough
This code is the same as _PWMLoopback.ino_, except with the PWM signal generation removed.

```C++
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
```

That's it! Your code should compile and be ready to upload to the [Teensy 3.2](https://www.pjrc.com/store/teensy32.html).

## Experiment
Upload your code to the Teensy and open the Serial Monitor. Turn on your transmitter. You should see a stream of 4 columns of data giving the measured PWM values. Try moving the sticks and switches while observing how these values change. You should notice the value move approximately over the 1000 to 2000 us PWM range as you move the sticks across their travel.

<img src="/images/rx_exp.png" alt="receiver exp" width="500">

You may see some values slightly below 1000 and above 2000 us. This is likely due to small differences in the clocks between the [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) and RC receiver. If we needed the value to be constrained within 1000 and 2000 us, we could add an _if_ statement to saturate the values on their bounds.

[![Experiment Video](https://img.youtube.com/vi/jbAeMbfYg4E/0.jpg)](https://www.youtube.com/watch?v=jbAeMbfYg4E)

## Wrap Up
In this experiment, we used [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) to measure PWM values from an RC receiver connected to a transmitter. We observed how we can measure pilot commands from the PWM data.

# Further Reading
1. [Wikipedia](https://en.wikipedia.org/wiki/Servo_control): quick background information on PWM.
2. [SparkFun](https://learn.sparkfun.com/tutorials/hobby-servo-tutorial/all): in depth information and pictures of the insides of servos.
3. [Princeton](https://www.princeton.edu/~mae412/TEXT/NTRAK2002/292-302.pdf): very in depth information about the PWM signals and servo circuits.

# Next Steps
Armed with the knowledge and experience that you gained with this tutorial, see if you can do the following:
1. Add some servos and try using the [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) and [Teensy PWM Backpack](http://bolderflight.com/products/teensy/pwm/) as a mixer. Can you make a single stick control two servos in the same direction? In opposite directions?
2. Try applying some stick shaping. Can you make the servos more sensitive to stick movements? Less sensitive? Try writing software to apply exponential, where the stick is less sensitive near the center and progressively more sensitive toward the extremes. Can you write a function so the amount of exponential can be easily changed?

# Next Tutorial
1. [SBUS to PWM Converter](https://github.com/bolderflight/SBUS-to-PWM): Use your knowledge of PWM and SBUS to create your own SBUS to PWM converter!
