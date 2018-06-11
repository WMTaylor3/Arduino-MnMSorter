/*	Arduino MnM culling machine.
*
*	by William Taylor https://github.com/WMTaylor3/MnM-Sorter
*	inspired by similar project by Dejan Nedelkovski, www.HowToMechatronics.com
*
*/

//Additional Libraries
#include <Servo.h>	//Required for interface to top and bottom servos.
#include <EEPROM.h>	//Required for storing RGB values to the Arduinos non-volatile memory.

//Definitions and variables relating to the color sensing aspect of the project.
#define colorSensorContolS0 6
#define colorSensorContolS1 5
#define colorSensorContolS2 4
#define colorSensorContolS3 3
#define colorSensorOutput 2
#define calibrateMode 11
//NOTE TO SELF - Refactor functions to use these as local variables.
int redMinimum;
int redMaximum;
int greenMinimum;
int greenMaximum;
int blueMinimum;
int blueMaximum;

//Definitions relating to the LEDs on top of the device.
#define LEDRED 9
#define LEDGREEN 10

//Definitions and variables relating to the servo control aspect of the project.
#define posDrop 48
#define posScan 85
#define posRecieve 143
#define posReject 40
#define posAccept 140
Servo topServo;
Servo bottomServo;


/*
*	Setup function of the program. Runs once when the device is booted.
*	No inputs or outputs.
*/
void setup()
{
	//Set pinModes for the above definitions.
	pinMode(colorSensorContolS0, OUTPUT);
	pinMode(colorSensorContolS1, OUTPUT);
	pinMode(colorSensorContolS2, OUTPUT);
	pinMode(colorSensorContolS3, OUTPUT);
	pinMode(colorSensorOutput, INPUT);
	pinMode(LEDGREEN, OUTPUT);
	pinMode(LEDRED, OUTPUT);
	pinMode(calibrateMode, INPUT);

	//Configure color sensors frequency scaling to 20%
	digitalWrite(colorSensorContolS0, HIGH);
	digitalWrite(colorSensorContolS1, LOW);

	//Attach top and bottom servos to their pins on the Arduino.
	topServo.attach(7);
	bottomServo.attach(8);

	//Begin serial communication to computer (used for testing only)
	Serial.begin(9600);

	//Read the stored values from the non-volatile memory on the Arduino.
	ReadValuesFromEEPROM();
}

/*
*	Main function of the program. Runs as a loop until device is powered down.
*	No inputs or outputs.
*/
void loop()
{
	//Determine if 'Calibrate' button is being pressed, if so, call the calibrate function.
	if (digitalRead(calibrateMode) == HIGH)
	{
		calibrate();
	}//End if

	 //Set both LEDs to off state.
	digitalWrite(LEDRED, LOW);
	digitalWrite(LEDGREEN, LOW);

	//Set the top servos position to be ready to receive an MnM from the charger.
	topServo.write(posRecieve);
	delay(500);

	//Move the newly recieved MnM to the scanning position underneath the color sensor.
	topServo.write(posScan);

	//Call function to determine whether the MnM is to be accepted or rejected.
	multipleColorDetermination();

	switch (multipleColorDetermination())
	{
		//If the MnM is valid, flash the green LED, move the bottom servo to align with the acceptance pile.
	case true:
		digitalWrite(LEDGREEN, HIGH);
		bottomServo.write(posAccept);
		delay(200);
		break;
		//If the MnM is invalid, flash the red LED, move the bottom servo to align with the rejection pile.
	case false:
		digitalWrite(LEDRED, HIGH);
		bottomServo.write(posReject);
		delay(200);
		break;
	}//End switch

	 //Move the top servo to align with the fallthrough point.
	topServo.write(posDrop);
	delay(500);
}

/*
*	Function for determining the final validity of the MnM.
*	Takes no input parameters.
*	Returns a boolean value representing whether the MnM is to be accepted (true) or rejected (false).
*	Operates by conducting five seperate scans of the MnM, if all result to false, return false. If all result to true, returns true.
*/
bool multipleColorDetermination()
{
	//Variable declaration.
	bool check[5];	// Stores the values of the five checks.

					//Loop until all 5 checks coalesce on a single value.
	while (true)
	{
		//Conduct five checks, 10 milliseconds apart.
		for (int i = 0; i < 5; i++)
		{
			check[i] = singleColorDetermination();
			delay(10);
		}//End for loop

		 //If all checks are true, return true. If all checks are false, return false. Otherwise, repeat loop.
		if (check[0] && check[1] && check[2] && check[3] && check[4])
		{
			return true;
		}//End if
		else if (!check[0] && !check[1] && !check[2] && !check[3] && !check[4])
		{
			return false;
		}//End if
	}//End while loop
}

/*
*	Function for determining the single validity of the MnM.
*	Takes no input parameters.
*	Returns a boolean value representing whether the MnM is to be accepted (true) or rejected (false).
*	Operates by conducting a single scan of the MnM. Is called five times from the multipleColorDetermination function.
*/
bool singleColorDetermination()
{
	//Variables declaration.
	int Red = 0;	//Stores the red value from the photodiode.
	int Green = 0;	//Stores the green value from the photodiode.
	int Blue = 0;	//Stores the blue value from the photodiode.

					//Configure the color sensor to read from the red photodiodes.
	digitalWrite(colorSensorContolS2, LOW);
	digitalWrite(colorSensorContolS3, LOW);
	//Read the red value.
	Red = pulseIn(colorSensorOutput, LOW);
	delay(10);

	//Configure the color sensor to read from the green photodiodes.
	digitalWrite(colorSensorContolS2, HIGH);
	digitalWrite(colorSensorContolS3, HIGH);
	//Read the green value.
	Green = pulseIn(colorSensorOutput, LOW);
	delay(10);

	//Configure the color sensor to read from the blue photodiodes.
	digitalWrite(colorSensorContolS2, LOW);
	digitalWrite(colorSensorContolS3, HIGH);
	//Read the blue value.
	Blue = pulseIn(colorSensorOutput, LOW);
	delay(10);

	//If the color values of the MnM are within the permissable range, return true, otherwise return false.
	if (Red >= redMinimum & Red <= redMaximum & Green >= greenMinimum & Green <= greenMaximum & Blue >= blueMinimum & Blue <= blueMaximum)
	{
		return false;
	}
	else
	{
		return true;
	}
}

/*
*	Function for calibrating the scanner, teaching it which MnM to accept or reject.
*	Takes no input parameters.
*	Operates by scanning the MnM multiple times, with a slight delay between each scan. These values are stored for comparison against in other functions.
*/
void calibrate()
{
	//Variable decalaration.
	int scannedNumber = 0;	//Number scanned from the MnM.

							//Sets the values to the opposite maximums possible so they are overwritten immediatly.
	redMinimum = 254;
	redMaximum = 1;
	greenMinimum = 254;
	greenMaximum = 1;
	blueMinimum = 254;
	blueMaximum = 1;

	//While the calibrate button is being held, keep the top servo at the position to recieve an MnM from the charger.
	while (digitalRead(calibrateMode) == HIGH)
	{
		topServo.write(posRecieve);
	}//End while

	 //Once the calibrate button has been released, move the top servo to the scanning position.
	topServo.write(posScan);

	//Conduct 1010 scanns of the MnM, discount the first 10 as the scanner needs time to return to base line after the servo has moved the MnM underneath it.
	for (int i = 0; i < 1010; i++)
	{
		if (i >= 10)
		{
			//Read Red values, set photodiode accordingly and store value in scannedNumber.
			digitalWrite(colorSensorContolS2, LOW);
			digitalWrite(colorSensorContolS3, LOW);
			scannedNumber = pulseIn(colorSensorOutput, LOW);
			//If the number is lower than the minimum or greater than the maximum, replace the appropriate value with the newly scanned one.
			if (scannedNumber < redMinimum)
			{
				redMinimum = scannedNumber;
			}//End if
			if (scannedNumber > redMaximum)
			{
				redMaximum = scannedNumber;
			}//End if
			delay(10);

			//Read Green values, set photodiode accordingly and store value in scannedNumber.
			digitalWrite(colorSensorContolS2, HIGH);
			digitalWrite(colorSensorContolS3, HIGH);
			scannedNumber = pulseIn(colorSensorOutput, LOW);
			//If the number is lower than the minimum or greater than the maximum, replace the appropriate value with the newly scanned one.
			if (scannedNumber < greenMinimum)
			{
				greenMinimum = scannedNumber;
			}//End if
			if (scannedNumber > greenMaximum)
			{
				greenMaximum = scannedNumber;
			}//End if
			delay(10);

			//Read Blue values, set photodiode accordingly and store value in scannedNumber.
			digitalWrite(colorSensorContolS2, LOW);
			digitalWrite(colorSensorContolS3, HIGH);
			scannedNumber = pulseIn(colorSensorOutput, LOW);
			//If the number is lower than the minimum or greater than the maximum, replace the appropriate value with the newly scanned one.
			if (scannedNumber < blueMinimum)
			{
				blueMinimum = scannedNumber;
			}//End if
			if (scannedNumber > blueMaximum)
			{
				blueMaximum = scannedNumber;
			}//End if
			delay(10);
		}
		else
		{
			delay(30);
		}//End else
	}//End for loop.

	 //Give each value an additional five units of headroom to ensure MnMs are scanned without error.
	redMinimum -= 5;
	redMaximum += 5;
	greenMinimum -= 5;
	greenMaximum += 5;
	blueMinimum -= 5;
	blueMaximum += 5;

	//Store the newly read values to the Arduinos non-volatile memory.
	StoreValuesToEEPROM();
}

/*
*	Function for storing the color values to the Arduinos EEPROM where they will survive power cycles.
*	Takes no input parameters.
*/
void StoreValuesToEEPROM()
{
	EEPROM.write(10, redMinimum);
	EEPROM.write(20, redMaximum);
	EEPROM.write(30, greenMinimum);
	EEPROM.write(40, greenMaximum);
	EEPROM.write(50, blueMinimum);
	EEPROM.write(60, blueMaximum);
}

/*
*	Reads new values from EEPROM and into the global variables.
*	Takes no input parameters.
*	Is called once upon the device powering up.
*/
void ReadValuesFromEEPROM()
{
	redMinimum = EEPROM.read(10);
	redMaximum = EEPROM.read(20);
	greenMinimum = EEPROM.read(30);
	greenMaximum = EEPROM.read(40);
	blueMinimum = EEPROM.read(50);
	blueMaximum = EEPROM.read(60);
}