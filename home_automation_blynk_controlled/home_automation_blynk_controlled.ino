/*************************************************************
Title         :   Home automation using blynk
Description   :   To control light's brigntness with brightness,monitor temperature , monitor water level in the tank through blynk app
Pheripherals  :   Arduino UNO , Temperature system, LED, LDR module, Serial Tank, Blynk cloud, Blynk App.
 *************************************************************/

// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "TMPLJQ3Lmr31"
#define BLYNK_DEVICE_NAME "Hmauto"
#define BLYNK_AUTH_TOKEN "xWhAWN-Jb_a5tmRTbVlHWbPUSm-MMUim"


// Comment this out to disable prints 
//#define BLYNK_PRINT Serial

#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "main.h"
#include "temperature_system.h"
#include "ldr.h"
#include "serial_tank.h"

char auth[] = BLYNK_AUTH_TOKEN;
bool heater_sw;
bool inlet_sw,outlet_sw;
unsigned int tank_volume;

BlynkTimer timer;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

// This function is called every time the Virtual Pin 0 state changes
/*To turn ON and OFF cooler based virtual PIN value*/
BLYNK_WRITE(COOLER_V_PIN)
{
   int value = param.asInt();
   //if cooler button is ON on blynk mobile application,then turn ON the cooler
  if (value)
  {
    cooler_control(ON);
    lcd.setCursor(7,0);
    lcd.print("CO_LR ON  ");
  }
  else
  {
   cooler_control(OFF);
   lcd.setCursor(7,0);
    lcd.print("CO_LR OFF  ");
  }
  
}
/*To turn ON and OFF heater based virtual PIN value*/
BLYNK_WRITE(HEATER_V_PIN )
{
    heater_sw = param.asInt();
   //if heater button is ON on blynk mobile application,then turn ON the cooler
  if (heater_sw)
  {
    heater_control(ON);
    lcd.setCursor(7,0);
    lcd.print("HT_R ON ");
  }
  else
  {
   heater_control(OFF);
   lcd.setCursor(7,0);
    lcd.print("HT_R OFF");
  }
  
}
/*To turn ON and OFF inlet vale based virtual PIN value*/
BLYNK_WRITE(INLET_V_PIN)
{
  inlet_sw=param.asInt();
  //if inlet valve buttons at logic high turn ON the inlet valve else OFF
  if(inlet_sw)
  {
    enable_inlet();
    //to print the status of value on clcd
     lcd.setCursor(7,1);
    lcd.print("IN_FL_ON  ");  
  }
else
{
     disable_inlet();
    //to print the status of value on clcd
     lcd.setCursor(7,1);
    lcd.print("IN_FL_OFF  ");  
}  


}
/*To turn ON and OFF outlet value based virtual switch value*/
BLYNK_WRITE(OUTLET_V_PIN)
{
  
  outlet_sw=param.asInt();
  //if inlet valve buttons at logic high turn ON the inlet valve else OFF
  if(outlet_sw)
  {
    enable_outlet();
    //to print the status of value on clcd
     lcd.setCursor(7,1);
    lcd.print("OT_FL_ON  ");  
  }
else
{
     disable_outlet();
    //to print the status of value on clcd
     lcd.setCursor(7,1);
    lcd.print("OT_FL_OFF  ");  
}  
}
/* To display temperature and water volume as gauge on the Blynk App*/  
void update_temperature_reading()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(TEMPERATURE_GAUGE,read_temperature());
   Blynk.virtualWrite(WATER_VOL_GAUGE,volume());

  
}



/*To turn off the heater if the temperature raises above 35 deg C*/
void handle_temp(void)
{
  //compare temperature with 35 and cheak if heater is ON
  if((read_temperature()> float(35)) && heater_sw)
  {
    //turn off the heater
    heater_sw=0;
     heater_control(OFF);
     //display notification on clcd
     lcd.setCursor(7,0);
    lcd.print("HT_R OFF  ");

    //to display notification on blynk
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN,"Temperature is above 35 degree Celcius\n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN,"Turning OFF the heater\n");
    

    // to reflect OFF on the heater button
    Blynk.virtualWrite(HEATER_V_PIN, OFF);

  }
  
}

/*To control water volume above 2000ltrs*/
void handle_tank(void)
{
  //compare the volume of water within 2000 ltr and cheak status of the inlet valve
  if((tank_volume < 2000) && (inlet_sw == OFF))
  {
    //enable inlet valve and print the status on clcd
     enable_inlet();
    //to print the status of value on clcd
     lcd.setCursor(7,1);
    lcd.print("IN_FL_ON  "); 
    inlet_sw=ON;

    //update the inlet button status on blynk app as ON 
    Blynk.virtualWrite(INLET_V_PIN ,ON);

    //print notification on virtual terminal
     Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN,"water volume is less then 2000\n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN,"Turning ON the INLET valve\n");
    
  }
   
   //cheak if the tank is full turn OFF the inlet valve
   if((tank_volume == 3000) && (inlet_sw == ON))
  {
    //enable inlet valve and print the status on clcd
     disable_inlet();
    //to print the status of value on clcd
     lcd.setCursor(7,1);
    lcd.print("IN_FL_OFF  "); 
    inlet_sw = OFF;

    //update the inlet button status on blynk app as ON 
    Blynk.virtualWrite(INLET_V_PIN ,OFF);

    //print notification on virtual terminal
     Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN,"water level is full");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN,"Turning OFF the INLET valve\n");
    
  }
   

}



void setup(void)
{
  //connecting the arduino to blink server
  Blynk.begin(auth);
/*initialize the lcd*/
    lcd.init();  
    /*turn the backlight */                   
    lcd.backlight();
    /*clear the clcd*/
    lcd.clear();
    /*cursor to the home */
    lcd.home();
  //initialising gardenlights as output pin
    init_ldr();
    //initialize the temp system
    init_temperature_system();
    lcd.setCursor(0,0);
    lcd.print("T=");


  //set cursor to second line to display volume of water
   lcd.setCursor(0,1);
    lcd.print("V="); 

    //initialising serial tank
    init_serial_tank();     
    //update temperature on the blynk app for every .5 sec
    timer.setInterval(500L,update_temperature_reading);   
}


void loop(void) 
{
  //control the brightness of garden lights using ldr sensor
 brightness_control(); 
  lcd.setCursor(2,0);
  String temperature;
  temperature = String (read_temperature(),2);
  //displaying the temp on clcd
  lcd.print(temperature);

//displaying volume on clcd
  tank_volume = volume();
  lcd.setCursor(2,1);
   lcd.print(tank_volume);

   //to cheak the thrshold temperature and controlling heater
   handle_temp();
  //to run the blynk application

  //to monitor volume of water and if < 2000 turn ON inlet valve 
  handle_tank();
  Blynk.run(); 
  timer.run();
}
