// WhiteBox Labs -- Tentacle Shield -- I2C interactive example
// https://www.whiteboxes.ch/tentacle
//
// How to use 4 (or 8 if using 2 Tentacle shields) Atlas Scientivic devices in I2C mode
// and interact with them via the serial monitor.
//
// This code is intended to work on all Arduinos. If using the Arduino Yun, connect
// to it's serial port. If you want to work with the Yun wirelessly, check out the respective
// Yun version of this example.
//
// USAGE:
//---------------------------------------------------------------------------------------------
// - Set all your EZO circuits to I2C before using this sketch.
//    - You can use the "tentacle-steup.ino" sketch to do so)
//    - Make sure each circuit has a unique I2C ID set 
// - Set host serial terminal to 9600 baud
//
// - To send a command, send the number of the i2c address, a colon and the command ending with a carriage return.
//
// - To issue a command, enter it directly to the console.
//
// 102:R<CR>
// 99:I<CR>
// 100:CAL<CR>
// 99:R<CR>
// 110:CAL,mid,7.00<CR>
//
// Possible commands:

// Command     Function                                  Manual page
// L           Enable / Disable or Query the LEDs        (pg.46)
// R           Returns a single reading                  (pg.47)
// T           Set or Query the temperature compensation (pg.48)
// CAL         Performs calibration                      (pg.49)
// SLOPE       Queries slope                             (pg.55)
// I           Device information                        (pg.56)
// STATUS      Retrieve status information               (pg.57)
// I2C         I2C address change                        (pg.58)
// SLEEP       Enter low power sleep mode                (pg.59)

//
//---------------------------------------------------------------------------------------------
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//---------------------------------------------------------------------------------------------

#include <Wire.h>                   // enable I2C.

unsigned long serial_host  = 9600;  // set baud rate for host serial monitor(pc/mac/other)

char sensordata[128];                // A 30 byte character array to hold incoming data from the sensors
byte computer_bytes_received = 0;   // We need to know how many characters bytes have been received
byte sensor_bytes_received = 0;     // We need to know how many characters bytes have been received
int channel;                        // INT pointer for channel switching - 0-7 serial, 8-127 I2C addresses
char *cmd;                          // Char pointer used in string parsing

char computerdata[128];              // we make a 48 byte character array to hold incoming data from a pc/mac/other.
byte code = 0;                      // used to hold the I2C response code.
byte in_char = 0;                   // used as a 1 byte buffer to store inbound bytes from the I2C Circuit.
int time;                   	      // used to change the dynamic polling delay needed for I2C read operations.


void setup() {                      // startup function
  Serial.begin(serial_host);	      // Set the hardware serial port.
  Wire.begin();			                // enable I2C port.
  intro();			                    // display startup message
}

void serialEvent() {               	// This interrupt will trigger when the data coming from the serial monitor(pc/mac/other) is received
  computer_bytes_received = Serial.readBytesUntil(13, computerdata, 128); 	
  // We read the data sent from the serial monitor(pc/mac/other) until we see a <CR>. 
  // We also count how many characters have been received
  computerdata[computer_bytes_received] = 0; 				        
  // We add a 0 to the spot in the array just after the last character we received. 
  // This will stop us from transmitting incorrect data that may have been left in the buffer
}

void loop() {                                 	// main loop
  if (computer_bytes_received != 0) {           // If computer_bytes_received does not equal zero    
    channel = atoi(strtok(computerdata, ":"));  // Let's parse the string at each colon
    cmd = strtok(NULL, ":");                    // Let's parse the string at each colon
    I2C_call();		                              // send to I2C
    computer_bytes_received = 0;                // Reset the var computer_bytes_received to equal 0
  }
}

void intro() {                                  // print intro
  Serial.flush();
  Serial.println(" ");
  Serial.println("READY_");
}

void I2C_call() {  			                        // function to parse and call I2C commands
  sensor_bytes_received = 0;                    // reset data counter
  memset(sensordata, 0, sizeof(sensordata));    // clear sensordata array;

  time = 10;                                     // minimal delay is 5 ms for read operations
  
  if (cmd[0] == 'C'  && cmd[4] != '?') time = 1800;
  if (cmd[0] == 'R') time = 800;
  if (cmd[0] == 'L' && cmd[2] != '?') time = 300;
                                 
  // if a command has been sent to Calibrate or take a Reading we wait 1400ms, 
  // so that the circuit has time to take the reading.
  // if any other command has been sent we wait only 300ms.
  
  Wire.beginTransmission(channel); 	            // call the circuit by its ID number.
  Wire.write(cmd);        		                  // transmit the command that was sent through the serial port.
  Wire.endTransmission();          	            // end the I2C data transmission.

  delay(time);                                  // wait for the device to process the command in 300 ms or 1400 ms

  code = 254;				                            // init code value to "< command pending"

  while (code == 254) {                         // in case the command takes longer to process, 
                                                // we keep looping here until we get a success or an error
    Wire.requestFrom(channel, 64, 1);           // call the circuit and request 48 bytes (this is more then we need).
    code = Wire.read();

    while (Wire.available()) {                  // are there bytes to receive.
      in_char = Wire.read();                    // receive a byte.

      if (in_char == 0) {                       // if we see that we have been sent a null command.
        Wire.endTransmission();                 // end the I2C data transmission.
        break;                                  // exit the while loop.
      }
      else {
        sensordata[sensor_bytes_received] = in_char;  // load this byte into our array.
        sensor_bytes_received++;
      }
    }                                           // end while (Wire.available)
    
    switch (code) {                  	          // switch case based on what the response code is.
      case 1:                       	          // decimal 1.
        //Serial.println("< Success");  	      // means the command was successful.  // Silence means everything is OK
        break;                        	       
      case 2:                                 	// decimal 2.
        Serial.println("< Command failed");    	// means the command has failed.
        break;                         	
      case 254:                      	          // decimal 254.
        Serial.println("< Command pending");   	// means the command has not yet been finished calculating.
        delay(100);                             // we wait for 200ms and give the circuit some time to complete the command
        break;                         	
      case 255:                      	          // decimal 255.
        Serial.println("< No data");   	        // means there is no further data to send.
        break;                         	
    }                                          // end switch
    
  }                                            // end while (code == 254)
  
  Serial.print(channel);                     // with two Atlas stamps we need to know from which device replies
  Serial.print(':');                         // same syntax as command with 'channel:reply'
  Serial.println(sensordata);	                 // print the data which inludes a <CRLF> \r\n at the end
}                                              // end I2CCall

