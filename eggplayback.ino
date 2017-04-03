/*
BASED ON 
 Example of reading the disk properties on OpenLog
 By: Nathan Seidle
 SparkFun Electronics
 Date: September 22nd, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

 This is an example of issuing the 'disk' command and seeing how big the current SD card is.
 
 Connect the following OpenLog to Arduino:
 RXI of OpenLog to pin 2 on the Arduino
 TXO to 3
 GRN to 4
 VCC to 5V
 GND to GND
 
 This example code assumes the OpenLog is set to operate at 9600bps in NewLog mode, meaning OpenLog 
 should power up and output '12<'. This code then sends the three escape characters and then sends 
 the commands to create a new random file called log###.txt where ### is a random number from 0 to 999.
 The example code will then read back the random file and print it to the serial terminal.
 
 This code assume OpenLog is in the default state of 9600bps with ASCII-26 as the esacape character.
 If you're unsure, make sure the config.txt file contains the following: 9600,26,3,0
 
 Be careful when sending commands to OpenLog. println() sends extra newline characters that 
 cause problems with the command parser. The new v2.51 ignores \n commands so it should be easier to 
 talk to on the command prompt level. This example code works with all OpenLog v2 and higher.
 
 */

#include <SoftwareSerial.h>

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//Connect TXO of OpenLog to pin 3, RXI to pin 2
SoftwareSerial OpenLog(3, 2); //Soft RX on 3, Soft TX out on 2
//SoftwareSerial(rxPin, txPin)

int resetOpenLog = 4; //This pin resets OpenLog. Connect pin 4 to pin GRN on OpenLog.
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int statLED = 13;
char inBuffer[101]; //100 plus an extra terminating NULL from the OpenLog

void setup() {                
  pinMode(statLED, OUTPUT);
  Serial.begin(9600);
  //Serial.println("OpenLog startup...");
  setupOpenLog(); //Resets logger and waits for the '<' I'm alive character
  //Serial.println("OpenLog online");
}

void loop() {
  //Read from the OpenLog
  gotoCommandMode(); //Puts OpenLog in command mode
  long int startTime = millis(),fileIdx=0,lastCmdPos=0;
  
  while(1) //keep playing the same file back over and over and over
     {
     while(read100Bytes("1.ebb",fileIdx) == 1) //read 100 bytes into the buffer, return 0 is the file is past EOF
        {
        lastCmdPos = play100Bytes(); //returns the location of the last detected full command
        if(lastCmdPos < 0)
           break;
        fileIdx += lastCmdPos;
        }

     fileIdx=0; //restart from the beggining!
     delay(5000); //with a small pause
     }
  
  //Infinite loop to show the file is done. 
  /*Serial.print("Took ");
  Serial.print(millis()-startTime);
  Serial.println(" mS");
  while(1) {
    digitalWrite(statLED, HIGH);
    delay(250);
    digitalWrite(statLED, LOW);
    delay(250);
  }*/
}

int play100Bytes()
{
int idx1=0,idx2=0,idx3=0,idx4=0;
char commandBuffer[101]; //plan for worst case, even if its not quite possible to have a command this large
while(1)
   {
   //find position of next '.' and also the '.' after it
   idx1=idx2;
   /*while(1)//find next '.'
      {
      if(inBuffer[idx1] == '.')
         break;
      else
         idx1++;
      if(idx1 > 100)
        {
        idx1 = -1; //no more!
        break;
        }
      }*/
   idx2=idx1+1; //only add one if idx1 is at a '.', which it won't be if its at zero (means the command starts byte 0)
   
   while(1) //find next '.'
      {
      if(inBuffer[idx2] == '.')
         break;
      else
         idx2++;
      
      if(idx2 > 100)
        {
        idx2 = -1; //no more!
        break;
        }
      }
   if(idx1 < 0 || idx2 < 0)
      return idx1;    //no more full commands to be found in this buffer
   else if(idx2 == idx1 + 1) //two ..'s in a row means EOF probably, so skip through
      return -1;
   else
      {
      //we have a command sitting between idx1 and ix2
      for(idx3 = idx1+1,idx4 = 0 ;idx3 < idx2; idx3++)
         {
         commandBuffer[idx4] = inBuffer[idx3];
         idx4++;
         }
      commandBuffer[idx4] = '\0'; //NULL terminate our command and SHIP IT OUT
      //send command
      Serial.write(commandBuffer);
      Serial.write("\r\n");
      //now wait for EBB to tell us 'OK\r\n'
      while(1)
         {
         if(Serial.available())
            if(Serial.read() == '\n') break;
         } 
      }
   }
}

//Setups up the software serial, resets OpenLog so we know what state it's in, and waits
//for OpenLog to come online and report '<' that it is ready to receive characters to record
void setupOpenLog(void) {
  pinMode(resetOpenLog, OUTPUT);
  OpenLog.begin(9600);

  //Reset OpenLog
  digitalWrite(resetOpenLog, LOW);
  delay(100);
  digitalWrite(resetOpenLog, HIGH);
  //delay(2000);
  //Wait for OpenLog to respond with '<' to indicate it is alive and recording to a file
  
  while(1) {
    if(OpenLog.available())
      if(OpenLog.read() == '<') break;
  }
}

//Reads the contents of a given file and dumps it to the serial terminal
//This function assumes the OpenLog is in command mode
int read100Bytes(char *fileName, long int startpos) {

  OpenLog.print("read ");
  OpenLog.print(fileName);
  OpenLog.print(" ");
  OpenLog.print(startpos);
  OpenLog.print(" 100\r");

  //The OpenLog echos the commands we send it by default so we have 'read log823.txt\r\n' sitting 
  //in the RX buffer. Let's try to not print this.
  while(1) {
    if(OpenLog.available())
      if(OpenLog.read() == '\n') break;
  }  

  //This will listen for characters coming from OpenLog and print them to the terminal
  //This relies heavily on the SoftSerial buffer not overrunning. This will probably not work
  //above 38400bps.
  
  int idx = 0,readcount = 0;
  char inChar;  
  while(1)
     {     
     if(OpenLog.available())
        {
        inChar = OpenLog.read();
        readcount++;
        //  inChar = '.';
        
        if(inChar == '>') //Is the openLog done giving us data? 
          break;
        else if(inChar == '\r' || inChar == '\n'); //don't process newlines
        else         
           {
           inBuffer[idx] = inChar;
           idx++;
           }
        
        if(idx > 100) //We have run out of buffer room. Save a spot for the NULL terminating character
          break;
        }
     }
     
  inBuffer[idx] = '\0';
  /* debug info 
  Serial.write(inBuffer);
  
  Serial.write('\n');
  Serial.print(idx);
  Serial.print(':');
  Serial.print(readcount); //looks like 3 bytes of overhead
  Serial.write('\n');
  */
  if(idx == 0)
    return 0; //File is empty!
  
  return 1; //success!
}

//This function pushes OpenLog into command mode
void gotoCommandMode(void) {
  //Send three control z to enter OpenLog command mode
  //Works with Arduino v1.0
  OpenLog.write(26);
  OpenLog.write(26);
  OpenLog.write(26);

  //Wait for OpenLog to respond with '>' to indicate we are in command mode
  while(1) {
    if(OpenLog.available())
      if(OpenLog.read() == '>') break;
  }
}
