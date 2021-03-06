
/// LIBRARIES THAT ARE IN USE (".h" is an abbreviation for header)
//#include <DHT.h> (temperature and humidity sensors)
//#include <SPI.h> // Library for work with devices that support SPI (serial periferal interface) 

//#include <LiquidCrystal.h>//LCD library
// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(8, 9, 4, 5, 6, 7); Here we create an object called lcd (object type is LiquidCrystal).

#include <SimpleTimer.h> //here is the SimpleTimer library
#include <NewPing.h> //using NewPing sonar library
#include <FlowMeter.h> //using a library for the flowmeter
#include <SoftwareSerial.h> //Relevant to blynk
#include <BlynkSimpleStream.h> //for blynk through serial or USB
//#include <SPI.h>
//#include <Ethernet.h>
//#include <BlynkSimpleEthernet.h> //blynk via ethernet connection
#include <OneWire.h> // needed for temperature module (waterproof DS18B20 Digital)

//#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#define BLYNK_PRINT DebugSerial
SoftwareSerial DebugSerial(2, 3); 
char auth[] = "3a4f13d3525e43749b5d0156c28d6ef7"; //Insert auth token between the " "

/// PINS THAT ARE USED IN THIS PROGRAM 
//For Arduino NANO 20,21 
#define TURBIDITY_SENSOR_PIN A1 //analogue 
#define FLOWMETER_PIN 2 // We need to use one of six interrupt pins. For Arduino Mega it is: 2,3,21,20,19,18
#define TEMP_K_TANK 3
#define TRIGGER_PIN 26  // distance measurer
#define ECHO_PIN 27     // distance measurer

#define CO2_VALVE_AL1_PIN 4 //Need relay (not all pins need relay. Some just send a signal)
#define CO2_VALVE_AL2_PIN 5
#define CO2_VALVE_AL3_PIN 6
#define CO2_VALVE_AL4_PIN 7 //Connect to the relay that will control the power to a pH regulating pump or valve

#define PUMP_A 9 //Need relay
#define PUMP_B 14 //Need relay

#define WATER_VALVE_AL1_PIN 11 //Need relay
#define WATER_VALVE_AL2_PIN 12
#define WATER_VALVE_AL3_PIN 13
#define WATER_VALVE_AL4_PIN 14

#define VALVE_K_RELAY_PIN 15 //needed to be replaced by actual pin-number // valve under K-tank //Need relay

#define FS_A_MIN 16 // defined floatswithces (random pins)
#define FS_A_MAX 17
#define FS_B_MIN 18
#define FS_B_MAX 19
#define FS_AL1_MAX 22 //Floatswitches for algae tanks.
#define FS_AL2_MAX 23
#define FS_AL3_MAX 24
#define FS_AL4_MAX 25

#define PH1_PIN A8 
#define PH2_PIN A9
#define PH3_PIN A10
#define PH4_PIN A11  // Connect the sensor's Po output to an analogue pin - whichever one you choose
#define PH_K_TANK_PIN A12 //pH sensor in K-tank

/// Constants // here we use #define to write down constants. Same is done for pins, but these are constants that are not pins.
#define MAX_DISTANCE 200 //defines max measuring distance in K-tank for ultrasonic sensor
#define MAX_WATER_VOLUME_K_TANK 56 // In liters. Than process will start when it is 50+6 liter
#define PERIOD_TO_CHECK_FLOWMETER 500 //check of volume for closing of valve
 
#define TURB_MEASURE_NUMBER 10 // How many times turbidity will be measured before comparing to the value named turbsetpoint
float STANDARD_TAKEOUT_FROM_AL = 1.0;// standard amount of water that is taken from AL-tank before switching to a next one. 
// Can be defined with floating point number

#define WATER_LEVEL_MEASURE_NUMBER 5 // How many times ultrasonic sensor will meauser before deciding on a current water level.

/// OBJECTS (object is a complex variable)

SimpleTimer timer; // Create a Timer object called "timer"!
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); //object controlling ultrasonic distance measuerer
FlowMeter Meter = FlowMeter(FLOWMETER_PIN, FS300A); // Constructor always have the same name as class
//Best to write in specific modell (after pin), namely FS300A 
OneWire temp_sensor(TEMP_K_TANK); // We created an object called temp_sensor 
// object type OneWire is defined in the library we included)
 
//LED widgets:
WidgetLED led1(V8); //Connect a LED widget to Virtual pin 8 in the app.
WidgetLED led2(V9); //
WidgetLED led3(V10); //
WidgetLED led4(V11); //
WidgetLED led_fs_al_tanks(V20);
WidgetLED led_BFP(V25);

/// VARIABLES
// Here variables were declared (they are read once when program starts)

float pH_setpoint_AL1 = 7.8; //default setpoint at startup set to 7.8
// (is correct that it's 78 and not 7.8 because slider can only work with integers (whole numbers)?
float pH_setpoint_AL2 = 7.8; //default setpoint at startup set to 7.8 (in these lines this variable is beeing created)
float pH_setpoint_AL3 = 7.8; 
float pH_setpoint_AL4 = 7.8; 
//Following lines were needed if would decide to have a LED that would light up,
// but it was decided to send a direct value to a ValueDisplay
// const float min_pH_K_tank = 7.4; //default set point. Min pH i CP tanken
// const float max_pH_K_tank = 8.4; //Max pH i CP tanken
// It makes sense to lie in the range typical for seawater, i.e. 7.5 to 8.3.
// (we can probably have a tiny bit wider borders)
// const float for min/max is written because we are not planning to change it within a program. 

// Variables: pH set to 10 by default to avoid CO2 valves activting on arduino boot 
// (not really necessary to be done, because checking goes after measuring) 
// (Anyway, initialization by real measuring in "setup()" would be better)
float pH1 = 10;
float pH2 = 10;
float pH3 = 10;
float pH4 = 10;
float pH_K_tank = 10;
float Po1;
float Po2;
float Po3;
float Po4;
float Po5;

int water_adding_valve_pin[] = {WATER_VALVE_AL1_PIN, WATER_VALVE_AL2_PIN, WATER_VALVE_AL3_PIN, WATER_VALVE_AL4_PIN};
// array of pin-numbers, where algae-valves  are connected
int current_tank = 0; //index of the item from water_adding_valve_pin-array,
// this item contains PIN, that is used now for the water-adding.
// int current_tank = 0 or 2 is going to be used once in two weeks (only to initialize the process of tankswitching)
int prev_level;
int valve_close_timer_ID; // Timer that is responsible for the function of closing of valve

int turbsetpoint = 1000;
int turbidity_measure_counter = 0; //I need a variable that is going to serve as a measurments counter
int total_turbidity_measurments = 0; //Here we are going to store a total number of turbidity measurments

int total_water_level_measurments = 0; //Set value to zero in order to get a new total for next ten measurments
int water_level_measure_counter = 0; //Set value to zero in order to start a new measurment cycle

boolean allow_work_of_K_valve = true;
int algae_FS_pin[] = {FS_AL1_MAX, FS_AL2_MAX, FS_AL3_MAX, FS_AL4_MAX};
// We create an array of constants that will contain the pins of all four FS in algaetanks

///Functions (there are always brackets after the function, f(x), brackets can also be empty)
// For conditions and cycles, the left bracket remains on the same line, and for functions goes to the next one.


void turb_control() 
{ 
    int turbidity = analogRead(TURBIDITY_SENSOR_PIN);
    if (turbidity_measure_counter < TURB_MEASURE_NUMBER) {
        total_turbidity_measurments += turbidity; // += means "add to this variable" there are also -=, /= and *=
        turbidity_measure_counter++; // this is an increment operator "++"
    }
    else {
        float average_turbidity = total_turbidity_measurments / TURB_MEASURE_NUMBER;
        Blynk.virtualWrite(V17, average_turbidity);
        if (average_turbidity < turbsetpoint) { //we get an average of ten measurments and compare it to setpoint
            Meter.reset(); // Here we reset the turn count for flowmeter and in the next line we open the valve.
            // Meter is an object assosiated with flow sensor. Reset is it's method
            digitalWrite(water_adding_valve_pin[current_tank], LOW);
            timer.enable(valve_close_timer_ID);
        }
        total_turbidity_measurments = 0; //Set value to zero in order to get a new total for next ten measurments
        turbidity_measure_counter = 0; //Set value to zero in order to start a new measurment cycle
    } 
}

void close_valve ()
{
    Meter.tick(PERIOD_TO_CHECK_FLOWMETER);
    // we call method "tick" from an object "meter"
    // Tick is prosessing of data. Here signals are converted to liters.
    double current_volume_flowmeter = Meter.getCurrentVolume();
    // here we declare a local variable. It's born in this frame. And it's gonna die here too.
    // And this method works because it belongs to an object Meter 
    // which is an instance of a class that is a part of a library (flowmeter.h) that we connected.
    if (current_volume_flowmeter >= STANDARD_TAKEOUT_FROM_AL) {
        digitalWrite(water_adding_valve_pin[current_tank], HIGH);
        timer.disable(valve_close_timer_ID);
        switch_tank();
    }
}

void switch_tank()
{
    if (current_tank == 0)
        current_tank = 1;  
    else if (current_tank == 1)
        current_tank = 0;
    else if (current_tank == 2)
        current_tank = 3;
    else if (current_tank == 3)
        current_tank = 2;
}

void maxlevel_K_tank()  // "void" because we don't expect result in physical world, but only in virtual 
{ 
    // /Measure water level, and if it is sufficient, open the valve under K-tank
    int volume = map(sonar.ping_cm(), 36, 3, 29, 70); //measurement of water level
    if (volume > 65) {
        if (led_BFP.getValue()==0) {
            led_BFP.on();
        }
        
    }
    else {
        if (led_BFP.getValue() > 0) {
            led_BFP.off();
        }
    }
    
    
    if (water_level_measure_counter < WATER_LEVEL_MEASURE_NUMBER) {
        total_water_level_measurments += volume; // += means "add to this variable" there are also -=, /= and *=
        water_level_measure_counter++; // this is an increment operator "++"
    }
    else {
        float average_water_level = total_water_level_measurments / WATER_LEVEL_MEASURE_NUMBER;
        if ((average_water_level >= MAX_WATER_VOLUME_K_TANK) && allow_work_of_K_valve) {
            // To ampersands mean a logical "and" // It causes both conditions to be fulfilled
            digitalWrite(VALVE_K_RELAY_PIN, LOW); 
            // If I need to open appropriate valve, then I need to apply voltage to the corresponding relay
        }
        total_water_level_measurments = 0; //Set value to zero in order to get a new total for next ten measurments
        water_level_measure_counter = 0; //Set value to zero in order to start a new measurment cycle
    } 
}
//in void maxleve_K_tank() we have two different operators. One is called If. Another is called If/else
    
void maxlevel_A_tank() 
{ 
    // If the upper float switch in the tank-A is closed, close the valve and switch on the pump in the tank-A
    if (digitalRead(FS_A_MAX) == LOW) { // If FS read is high then...
        digitalWrite(VALVE_K_RELAY_PIN, HIGH);
        digitalWrite(PUMP_A, LOW);
    }
}

void minlevel_A_tank() 
{
    // If the lower float swtich in the tank-A is opened, turn off the pump in the tank-A
    if (digitalRead(FS_A_MIN) == HIGH) {
        digitalWrite(PUMP_A, HIGH);
    }
}

void maxlevel_B_tank() 
{
    // If the upper float switch in the tank-B is closed, turn on the pump in the tank-B
    if (digitalRead(FS_B_MAX) == LOW) {
        digitalWrite(PUMP_B, LOW);
    }
}

void minlevel_B_tank() 
{
    //If the lower float switch in the tank-B is closed, turn off the pump in the tank-B
    if (digitalRead(FS_B_MIN) == HIGH) { // here "high" means "contact has open"
        // delay(3000); (If we will not come up with anything better, then can do this.
        // It is actually not necessary to do it at all,
        // because lower floatswitch is only responsible for turning the pump off.
        digitalWrite(PUMP_B, HIGH);
    }
}

void maxlevel_algae_tanks() 
{
    allow_work_of_K_valve = true;
    for (int al_tank_index = 0; al_tank_index < 4; al_tank_index++) { // Is called cycle with a counter.
        // That is, al_tank_index first turn to the tank zero,
        // then consequently turn to the tanks one, two and three (therefore it is written "> 4")
        if (digitalRead(algae_FS_pin[al_tank_index]) == LOW) {
            // If the upper float switch in the tank-A is closed,
            // close the valve and switch on the pump in the tank-A
            digitalWrite(VALVE_K_RELAY_PIN, HIGH);
            digitalWrite(PUMP_A, HIGH);
            digitalWrite(PUMP_B, HIGH);
            allow_work_of_K_valve = false;
            // These five lines make it so that we consequently shut down the whole bottom part of RAS,
            // and then forbid to turn it on again
            if (led_fs_al_tanks.getValue() == 0) {
                led_fs_al_tanks.on();
            } 
        }   
    }
    if (allow_work_of_K_valve) {
        if (led_fs_al_tanks.getValue() > 0) {
            led_fs_al_tanks.off();
        }
    } 
}

// Metod param.asInt() reads the parameter from "step H" widget as a whole number.
// Respectively: param.asFloat() will read it as a float

/*
BLYNK_WRITE(6)  // Changes are set by the actions application itself (this is reading from a widget)
{ 
    STANDARD_TAKEOUT_FROM_AL = param.asFloat(); 
}

BLYNK_WRITE(12)  // Connect a step H widget to virtual pin 12 in the app.
{
    pH_setpoint_AL1 = param.asFloat(); 
} 

BLYNK_WRITE(13)
{
    pH_setpoint_AL2 = param.asFloat(); 
}

BLYNK_WRITE(14)
{
    pH_setpoint_AL3 = param.asFloat(); 
}

BLYNK_WRITE(15)
{
    pH_setpoint_AL4 = param.asFloat(); 
}


void blynker1() { //Writes the setpoint value to a gague widget.
// Connect the gague widget to virtual pin 1: to show on the screen what is the setpoint
    Blynk.virtualWrite(V21, pH_setpoint_AL1);  
}
void blynker2() {
  Blynk.virtualWrite(V22, pH_setpoint_AL2);
}
void blynker3() {
    Blynk.virtualWrite(V23, pH_setpoint_AL3);
}
void blynker4() {
  Blynk.virtualWrite(V24, pH_setpoint_AL4);
}
void blynker5() {
  Blynk.virtualWrite(V26, STANDARD_TAKEOUT_FROM_AL); 
}
*/

BLYNK_WRITE(6)  // Changes are set by the actions application itself (this is reading from a widget)
{ 
    STANDARD_TAKEOUT_FROM_AL = param.asFloat(); 
    Blynk.virtualWrite(V26, STANDARD_TAKEOUT_FROM_AL); //This function takes over blynker1 functionality.
    // Blynker1 used to be triggered by timer. Now it is going to only be triggered if we press on step-H
    // (steppers for pH-values, and standard takeout from Al)
}
BLYNK_WRITE(12)  // Connect a step H widget to virtual pin 12 in the app.
{
    pH_setpoint_AL1 = param.asFloat();
    Blynk.virtualWrite(V21, pH_setpoint_AL1); 
} 
BLYNK_WRITE(13)
{
    pH_setpoint_AL2 = param.asFloat(); 
    Blynk.virtualWrite(V22, pH_setpoint_AL2);
}
BLYNK_WRITE(14)
{
    pH_setpoint_AL3 = param.asFloat(); 
    Blynk.virtualWrite(V23, pH_setpoint_AL3);
}
BLYNK_WRITE(15)
{
    pH_setpoint_AL4 = param.asFloat(); 
    Blynk.virtualWrite(V24, pH_setpoint_AL4);
}

// modified map function for float values. Now we don't need an additional variables (pHm1,pHm2,pHm3,pHm4)
float map_float (float value,float fromLow, float fromHigh, float toLow, float toHigh)
{
    return (toLow + (value - fromLow) * ((toHigh - toLow) / (fromHigh - fromLow))); 
    //  Logic 
    //4 float k = (toHigh - toLow) / (fromHigh - fromLow);
    //3 float len_v = value - fromLow;
    //2 float new_len = len_v * k;
    //1 return (toLow + new_len); 
}  

void temp_measure_k_tank()
{   // Will give value in whole numbers (need extra research in order to get a float value) 
    //float temp_k; // variable storing current temperature
    //temp_k = -5;  //later we need to substitute this value with a real number
    byte data[2]; //we create an array called data to write data from temp_sensor
    temp_sensor.reset(); 
    temp_sensor.write(0xCC);
    temp_sensor.write(0x44);
    delay(750);
    temp_sensor.reset();
    temp_sensor.write(0xCC);
    temp_sensor.write(0xBE);
    data[0] = temp_sensor.read(); 
    data[1] = temp_sensor.read();
    float Temp = Temp = ((data[1]<< 8)+data[0])/16.0;
    // Magic happens and we get a float that shows the current temperature in 'C, written in a variable temp. 
    Blynk.virtualWrite(V18, Temp);
}

void setupBlynk() //Here we set initial values in widget in blynk. Just to make it pretty.
{
    Blynk.virtualWrite(V6, STANDARD_TAKEOUT_FROM_AL);
    Blynk.virtualWrite(V12, pH_setpoint_AL1);
    Blynk.virtualWrite(V13, pH_setpoint_AL2);
    Blynk.virtualWrite(V14, pH_setpoint_AL3);
    Blynk.virtualWrite(V15, pH_setpoint_AL4);
    
    Blynk.virtualWrite(V26, STANDARD_TAKEOUT_FROM_AL);
    Blynk.virtualWrite(V21, pH_setpoint_AL1);
    Blynk.virtualWrite(V22, pH_setpoint_AL2);
    Blynk.virtualWrite(V23, pH_setpoint_AL3);
    Blynk.virtualWrite(V24, pH_setpoint_AL4);
    ph(); //It works once and set initial values for pH (it will be called after 24 seconds anyway).
}



void ph()
{
    Po1 = (1023 - analogRead(PH1_PIN)); // it is done convert value from analogue sensor to 
    Po2 = (1023 - analogRead(PH2_PIN));
    Po3 = (1023 - analogRead(PH3_PIN));
    Po4 = (1023 - analogRead(PH4_PIN));
    Po5 = (1023 - analogRead (PH_K_TANK_PIN));
  
    //Serial.print(Po1); //This is the raw voltage value for the pH module
    //Serial.print(Po2); //This is the raw voltage value for the pH module
    //Serial.print(Po3); //This is the raw voltage value for the pH module
    //Serial.print(Po4); //This is the raw voltage value for the pH module
    //Calibration values:
    //405@pH7 // These values were checked/calibrated manually
    //290@ph4

    //Serial.print(", ph =");
    float pHm1 = map(Po1, 290, 406, 400, 700);
    float pH1 = (pHm1/100);
    float pHm2 = map(Po2, 290, 406, 400, 700);
    float pH2 = (pHm2/100);
    float pHm3 = map(Po3, 290, 406, 400, 700);
    float pH3 = (pHm3/100);
    float pHm4 = map(Po4, 290, 406, 400, 700);
    float pH4 = (pHm4/100);
    float pHm_K_tank = map(Po5, 290, 406, 400, 700);
    float pH_K_tank = (pHm_K_tank/100);

    Blynk.virtualWrite(V1, pH1);
    Blynk.virtualWrite(V2, pH2);
    Blynk.virtualWrite(V3, pH3);
    Blynk.virtualWrite(V4, pH4);
    Blynk.virtualWrite(V5, pH_K_tank);
  
// With the help of an extra if function for leds, the amount of information that goes to the blynk is minimized.
    if (pH1 > (pH_setpoint_AL1)) {
        digitalWrite(CO2_VALVE_AL1_PIN, LOW);
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:ON*"); 
        if (led1.getValue()==0) { //If led was off, it will be turned on.
// If the led is already ON, program will not send the signal "to switch on" to the blynk and thus will send less data.
            led1.on();
        } 
    }
    else {
        digitalWrite(CO2_VALVE_AL1_PIN, HIGH);
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:OFF");
        if (led1.getValue() > 0) {
            led1.off();
        }      
    }
    
    if (pH2 > (pH_setpoint_AL2)) {  // This if/else statement turns the valve on if the pH value is above the setpoint,
        // or off if it's below the setpoint. Modify according to your need.
        digitalWrite(CO2_VALVE_AL2_PIN, LOW);
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:ON*");
        if (led2.getValue()==0) {
            led2.on();
        } 
    }
    else {
        digitalWrite(CO2_VALVE_AL2_PIN, HIGH);
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:OFF");
        if (led2.getValue() > 0) {
            led2.off();
        }       
    }
    
    if (pH3 > (pH_setpoint_AL3)) {  // This if/else statement turns the valve on if the pH value is above the setpoint,
        // or off if it's below the setpoint. Modify according to your need.
        digitalWrite(CO2_VALVE_AL3_PIN, LOW); 
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:ON*");
        if (led3.getValue()==0) {
            led3.on();
        } 
    }
    else {
        digitalWrite(CO2_VALVE_AL3_PIN, HIGH);
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:OFF");
        if (led3.getValue() > 0) {
            led3.off();
        }      
    }
    
    if (pH4 > (pH_setpoint_AL4)) {
        digitalWrite(CO2_VALVE_AL4_PIN, LOW);
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:ON*");
        if (led4.getValue()==0) {
            led4.on();
        } 
    }
    else {
        digitalWrite(CO2_VALVE_AL4_PIN, HIGH);// lcd.setCursor(8, 0);
        // lcd.setCursor(8, 0);
        // lcd.print("CO2:OFF");
        if (led4.getValue() > 0) {
            led4.off();
        }      
    }

void MeterISR() 
{
    // let our flow meter count the pulses
    // https://github.com/sekdiy/FlowMeter/blob/master/examples/Simple/Simple.ino 
    Meter.count();
}

void setup()
{
    Serial.begin(9600); // initialize serial communications at 9600 bps
    //Change serial to debug serial to connecti directly via usb --> DebugSerial.begin(9600) 
    //pinMode(FLOWMETER_PIN // we have it as attachInterrupt(2, MeterISR, RISING) at the end of void setup()
    pinMode(TRIGGER_PIN, OUTPUT);         // distance measurer
    pinMode(ECHO_PIN, INPUT);            // distance measurer
    pinMode(CO2_VALVE_AL1_PIN, OUTPUT);
    pinMode(CO2_VALVE_AL2_PIN, OUTPUT);
    pinMode(CO2_VALVE_AL3_PIN, OUTPUT);
    pinMode(CO2_VALVE_AL4_PIN, OUTPUT); //Connect to the relay that will control the power to a pH regulating pump or valve
    pinMode(WATER_VALVE_AL1_PIN, OUTPUT);
    pinMode(WATER_VALVE_AL2_PIN, OUTPUT);
    pinMode(WATER_VALVE_AL3_PIN, OUTPUT);
    pinMode(WATER_VALVE_AL4_PIN, OUTPUT);
    pinMode(VALVE_K_RELAY_PIN, OUTPUT); //needed to be replaced by actual pin-number // valve under K-tank
    pinMode(FS_A_MIN, INPUT_PULLUP);          // defined floatswithces (random pins)
    pinMode(FS_A_MAX, INPUT_PULLUP);
    pinMode(FS_B_MIN, INPUT_PULLUP);
    pinMode(FS_B_MAX, INPUT_PULLUP);
    pinMode(FS_AL1_MAX, INPUT_PULLUP); //Floatswitches for algae tanks
    pinMode(FS_AL2_MAX, INPUT_PULLUP);
    pinMode(FS_AL3_MAX, INPUT_PULLUP);
    pinMode(FS_AL4_MAX, INPUT_PULLUP);
    pinMode(PUMP_A, OUTPUT);
    pinMode(PUMP_B, OUTPUT); 
    // Analog channels are not configurable for INPUT or OUTPUT. It is only relevant for digital pins
    // Howerver, analog pins can be configured to work as digital pins.
    digitalWrite(CO2_VALVE_AL1_PIN, HIGH); //Turns off relays as arduino boots up
    digitalWrite(CO2_VALVE_AL2_PIN, HIGH);
    digitalWrite(CO2_VALVE_AL3_PIN, HIGH);
    digitalWrite(CO2_VALVE_AL4_PIN, HIGH);
    digitalWrite(WATER_VALVE_AL1_PIN, HIGH);
    digitalWrite(WATER_VALVE_AL2_PIN, HIGH);
    digitalWrite(WATER_VALVE_AL3_PIN, HIGH);
    digitalWrite(WATER_VALVE_AL4_PIN, HIGH);
    digitalWrite(PUMP_A, HIGH);
    digitalWrite(PUMP_B, HIGH);
    
    Blynk.begin(Serial, auth);
    
    // lcd.begin(16, 2); // set up the LCD's number of columns and rows: 
    // if (ethbutton == LOW) {
    //Blynk.begin(auth, "blynk-cloud.com");
    //timer.setInterval(2100, blynker1); //This timer has updated a new value of the standard setpoint every two seconds
    //timer.setInterval(4800, blynker1); 
    //timer.setInterval(3250, blynker2);
    //timer.setInterval(4450, blynker3);
    //timer.setInterval(5220, blynker4);
    //timer.setInterval(5150, blynker5);
 
    //the number for the timers sets the interval for how frequently the function is called.
    // Keep it above 1000 to avoid spamming the server.
  
    timer.setInterval(24000, ph);
    timer.setInterval(60000, turb_control); // timer will call a turb_control function every minute
    timer.setInterval(5000, temp_measure_k_tank);
    timer.setInterval(1000, maxlevel_K_tank); 
    valve_close_timer_ID = timer.setInterval(PERIOD_TO_CHECK_FLOWMETER, close_valve); 
    // Getting a timer ID which is responsible for closing of valve
    // Every half second it should call a function close_valve, but in next line it is disabled before needed
    timer.disable(valve_close_timer_ID);

    // enable a call to the 'interrupt service handler' (ISR) on every rising edge at the interrupt pin
    // do this setup step for every ISR you have defined, depending on how many interrupts you use
    attachInterrupt(2, MeterISR, RISING); //If interrupt pin is changed then this constant has to be changed to.
    // sometimes initializing the gear generates some pulses that we should ignore
    setupBlynk();
    Meter.reset();
}


void loop() // These are functions that don't have a timer and some other stuff being called every tact
{ 
    Blynk.run(); 
    timer.run(); //For those functions that are registered in the loop, they are called with each tact,
    // that is we don't need to assign a specific set.interval function
    maxlevel_A_tank(); // However we still need to write those functions in void.loop, namely:
    minlevel_A_tank();
    maxlevel_B_tank();
    minlevel_B_tank();
    maxlevel_algae_tanks();
}
