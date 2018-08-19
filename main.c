#include <iostm8s105s4.h>
#include <intrinsics.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stm8def.h"
#include "i2c_drv.h"
#include "diverses.h"
#include "ds3231M_RTC.h"
#include "LCD\lcd-routines.h"
#include "SSD1306\SSD1306.h"
#include "LCD_HT1621\LCDHT1621.h"
#include "OLED\oled.h"
#include "nokia5110\nokia5110.h"
#include "drehgeber\drehgeber.h"
#include "onewire.h"
#include "ds18b20.h"

// Masterclock 
#define F_MASTER_HZ     16000000UL

//I2C Standard speed 400 kbit/s
#define F_I2C_HZ    400000UL


int main( void )
{
  
  
  char Temp_Text[] ="  ";
  // int temperature = 20; 
  s16 DS18B20_Temperature;
  u8 tmp[4]; t_i2c_status status = I2C_SUCCESS;
  int dreh = 0; int dreh1 =0; int dreh2 =0;
  int digit, decimal, temp; 
  u8 rom[8];
  
// setup

  //  Ensure the clocks are running at full speed.
  CLK_CKDIVR = 0x00; 

// I2C bus init  
  i2c_master_init(F_MASTER_HZ, F_I2C_HZ);
  __enable_interrupt();
  
// UART init
  Initialisierung_UART2();
  delay_ms(500);
  uart_sendstr("\n\nUART Uhr stellen?\n\n");
  
// OLED init
  oled_init(OLED_DISP_ON);
  oled_clrscr();
  oled_gotoxy(0,1);
  oled_puts("OLED Uhr stellen?\0");
  
// I2C LCD init
  lcd_init(); lcd_clear();
  lcd_string("LCD Uhr stellen?");
  
// Nokia LCD Init
  InitialisePort(); LcdClearScreen();
  LcdWriteString("Nokia Uhr stellen?");

// DS18B20 init and read ROM-ID
  temp = DS18B20_All_init();
  if (temp)
  {
    lcd_string("error");
    delay_ms(1000);
  }
  
// DS18B20 ROM-Kennung anzeigen
  oled_gotoxy(0,4);
  DS18B20_Read_ROM_ID(&rom[0]);
  for (u8 z=0; z < 8; z++)
  {
    ItoHex(rom[z],Temp_Text); 
    oled_puts (Temp_Text);
    uart_sendstr(Temp_Text);
    uart_sendstr(" ");
  };
  uart_sendstr("\n\n");
  
// HT1621B-LCD init  
  LCDHT1621_init();
  LCDOn(); 
  LCDString("STELLEN");  
  
// wait to see all display-messages
  delay_ms(5000);
  
 
//hier fehlt noch die Tastenabfrage, Code ist Pseudoblabla
  int Stellen = 0x00;
  if (Stellen != 0) {

    // noch feste Zeit mangels Eingabe der Uhrzeit mit Dreh-Encoder
    RTC_sec = 0;
    RTC_min = 46;
    RTC_hour = 13;
    
    tmp[0] = dec2hex(RTC_sec);
    tmp[1] = dec2hex(RTC_min);
    tmp[2] = dec2hex(RTC_hour);
    status = i2c_wr_reg(0xD0, 0x00, tmp, 3);
    if (status == I2C_SUCCESS) {
      uart_sendstr("UART Uhr gestellt! \n \n"); 
      oled_puts("OLED Uhr gestellt!\0");
      lcd_string("LCD Uhr gestellt!        "); 
    }
  }
  delay_ms(1000);
  
// gleich geht es los  
  lcd_clear();
  oled_clrscr();
  init_drehgeber();
  
// loop
  while(1){

    // Temperatur lesen und ausgeben
    status = i2c_rd_reg(0xD0, 0x11, tmp, 2);
    if(status == I2C_SUCCESS)
    {
	RTC_temp =  (s16) ((u16)((tmp[0]&0x7F)<<2) | ((tmp[1]>>6)&0x03));
	if(tmp[0] & 0x80) RTC_temp = -RTC_temp;
    }
    //temperature = RTC_temp*25/100;
    ItoA((RTC_temp>>2), Temp_Text);
    lcd_setcursor(0,1);
    oled_gotoxy(0,3);
    lcd_string  (Temp_Text);
    oled_puts(Temp_Text);
    lcd_string(",");
    oled_puts(",");
    ItoA((RTC_temp&0x03)*25/10,Temp_Text);
    lcd_string  (Temp_Text);
    oled_puts(Temp_Text);
    lcd_string  ("\337C ");
    oled_puts("°C");
    
    // Uhrzeit lesen und ausgeben
    status = i2c_rd_reg(0xD0, 0x00, tmp, 3);
    if(status == I2C_SUCCESS)
      {
       RTC_sec = hex2dec(tmp[0]);
       RTC_min = hex2dec(tmp[1]);
       RTC_hour = hex2dec(tmp[2]);
      }
    oled_gotoxy(0,1);
    if (RTC_hour < 10) 
    {
      lcd_string("0");
      oled_puts("0"+0);
    }
    ItoA(RTC_hour, Temp_Text);
    lcd_string  (Temp_Text);
    oled_puts(Temp_Text);
    lcd_string  (":");
    oled_puts(":"+0);
    if (RTC_min < 10)     
    {
      lcd_string("0");
      oled_puts("0"+0);
    }

    ItoA(RTC_min, Temp_Text);
    lcd_string  (Temp_Text);
    oled_puts(Temp_Text);
    lcd_string  (":");
    oled_puts(":"+0);
    if (RTC_sec < 10) 
    {
      lcd_string("0");
      oled_puts("0"+0);
    }
    ItoA(RTC_sec, Temp_Text);
    lcd_string  (Temp_Text);
    oled_puts(Temp_Text);
    //LCDdisplayNumber(RTC_sec);
    
  if (RTC_sec == 0) oled_invert(YES);
   else oled_invert(NO);
   LCDString("        ");
  if (RTC_sec%10 == 0) LCDString("PUUP "); 
   else LCDString("PIEP ");

// Temperatur DS18B20 lesen und ausgeben
    DS18B20_All_convert();
    lcd_setcursor(5,2);
    DS18B20_All_Read_Temp(&DS18B20_Temperature);
    if (DS18B20_Temperature <0) 
      {
        lcd_string("-");
        DS18B20_Temperature = - DS18B20_Temperature;
      }
    temp = DS18B20_Temperature*10/16;            // in 1/10 Grad umrechnen
    decimal = temp / 10;                         // Vorkomma ohne Vorzeichen
    digit = temp % 10;                          // Nachkomma ohne Vorzeichen   
    ItoA(decimal, Temp_Text);
    lcd_string  (Temp_Text); 
    lcd_string (",");
    ItoA(digit, Temp_Text);
    lcd_string  (Temp_Text); 
    lcd_string  ("\337C ");

    // Drehencoder auswerten durch Ausgabe des Wertes von TIM1-counter    
    dreh1 = (TIM1_CNTRH << 8);
    dreh2 = TIM1_CNTRL;
    dreh = dreh2 + dreh1;
    ItoA(dreh, Temp_Text);
//    oled_gotoxy(0,5);
//    oled_puts("       ");
//    oled_gotoxy(0,5);
    lcd_setcursor(0,2);
    lcd_string("   ");
    lcd_setcursor(0,2);
    lcd_string(Temp_Text);

    
    //   OLED_Update();  






/*
 // nach i2c devices scannen und gefundene devices ausgeben
    int i, j;
    for (i = 0; i < 127; i++) 
    {
      j = i2c_test_slave(i*2);
      if (j == 0) 
      {
        ItoA(i,Temp_Text);
        uart_sendstr(Temp_Text);
        uart_sendstr("   x \n");
      }
    } 
    //
    // 39 => 0x4E PCF8574(LCD); 
    // 60 => 0x78 OLED-Display;
    // 72 => 0x90 PCF8591; 
    // 80, 104 => 0xA0 24C32 und 0xD0 DS1307; 
    // 87, 104 => 0xAE 24C32 und 0xD0 DS3231 
*/

// rumwarten, damit die Anzeige nicht so oft aktualisiert wird
    delay_ms(1000);
    
  }
    
}
