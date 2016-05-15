/* hello_lcd.c  */

#include "16F690.h"
#include "int16Cxx.h"
#pragma config |= 0x00D4 
#pragma char X @ 0x115

/* I/O-pin definitions                               */ 
/* change if you need a pin for a different purpose  */
#pragma bit RS  @ PORTB.4
#pragma bit EN  @ PORTB.6

#pragma bit D7  @ PORTC.3
#pragma bit D6  @ PORTC.2
#pragma bit D5  @ PORTC.1
#pragma bit D4  @ PORTC.0

void delay( char ); // ms delay function
void lcd_init( void );
void lcd_putchar( char );
char text1( char );
char text2( char );
void init_serial( void );
void init_interrupt( void );
void putchar( char ch );
void printf(const char *string, char variable);

bit receiver_flag;   /* Signal-flag used by interrupt routine   */
char receiver_byte;  /* Transfer Byte used by interrupt routine */

#pragma origin 4
interrupt int_server( void  ) /* the place for the interrupt routine */
{
  int_save_registers
  /* New interrupts are automaticaly disabled            */
  /* "Interrupt on change" at pin RA1 from PK2 UART-tool */
  
  if( PORTA.1 == 0 )  /* Interpret this as the startbit  */
    {  /* Receive one full character   */
      char bitCount, ti;
      /* delay 1,5 bit 156 usec at 4 MHz         */
      /* 5+28*5-1+1+2+9=156 without optimization */
      ti = 28; do ; while( --ti > 0); nop(); nop2();
      for( bitCount = 8; bitCount > 0 ; bitCount--)
       {
         Carry = PORTA.1;
         receiver_byte = rr( receiver_byte);  /* rotate carry */
         /* delay one bit 104 usec at 4 MHz       */
         /* 5+18*5-1+1+9=104 without optimization */ 
         ti = 18; do ; while( --ti > 0); nop(); 
        }
      receiver_flag = 1; /* A full character is now received */
    }
  RABIF = 0;    /* Reset the RABIF-flag before leaving   */
  int_restore_registers
  /* New interrupts are now enabled */
}


void main( void)
{
    /* I/O-pin direction in/out definitions, change if needed  */
	ANSEL=0; 	//  PORTC digital I/O
	ANSELH=0;
	TRISC = 0b1111.0000;  /* RC3,2,1,0 out*/
    TRISB.4=0; /* RB4, RB6 out */
    TRISB.6=0;	

    char i, choice;
    lcd_init();
	
	init_serial();
	init_interrupt();

    RS = 1;  // LCD in character-mode
    // display the 8 char text1() sentence
    for(i=0; i<8; i++) lcd_putchar(text1(i)); 

   // reposition to "line 2" (the next 8 chars)
    RS = 0;  // LCD in command-mode
    lcd_putchar( 0b11000000 );
  
    RS = 1;  // LCD in character-mode
    // display the 8 char text2() sentence
    for(i=0; i<8; i++) lcd_putchar(text2(i)); 
   
    //while(1) nop();
	
	delay(1000); 
	printf("Menu: 1, 2, 3\r\n",0);
	
	while(1)
   {
   printf("asdaskudfgou\r\n",0);
     if( receiver_flag ) /* Character received? */ 
      {
	  printf("Menu: 1, 2, 3\r\n",0);
        choice = receiver_byte; /* get Character from interrupt routine */
        receiver_flag = 0;      /* Character now taken - reset the flag */

        switch (choice)
         {
          case '1':
           PORTC.0 = !PORTC.0;
           printf("%c LED0 now: ON\r\n", choice);
		   text1('1');
           break;
  
          default:
           printf("%c You must choose between: 1, 2, 3\r\n", choice);
         }
      }     
     /* if no Character is received we always loop here */
	 
   }
}



/* *********************************** */
/*            FUNCTIONS                */
/* *********************************** */


char text1( char x)   // this is the way to store a sentence
{
   skip(x); /* internal function CC5x.  */
   #pragma return[] = "Hello wo"    // 8 chars max!
}

char text2( char x)   // this is the way to store a sentence
{
   skip(x); /* internal function CC5x.  */
   #pragma return[] = "rld!    "    // 8 chars max!
}


void lcd_init( void ) // must be run once before using the display
{
  delay(40);  // give LCD time to settle
  RS = 0;     // LCD in command-mode
  lcd_putchar(0b0011.0011); /* LCD starts in 8 bit mode          */
  lcd_putchar(0b0011.0010); /* change to 4 bit mode              */
  lcd_putchar(0b00101000);  /* two line (8+8 chars in the row)   */ 
  lcd_putchar(0b00001100);  /* display on, cursor off, blink off */
  lcd_putchar(0b00000001);  /* display clear                     */
  lcd_putchar(0b00000110);  /* increment mode, shift off         */
  RS = 1;    // LCD in character-mode
             // initialization is done!
}


void lcd_putchar( char data )
{
  // must set LCD-mode before calling this function!
  // RS = 1 LCD in character-mode
  // RS = 0 LCD in command-mode
  // upper Nybble
  D7 = data.7;
  D6 = data.6;
  D5 = data.5;
  D4 = data.4;
  EN = 0;
  nop();
  EN = 1;
  delay(5);
  // lower Nybble
  D7 = data.3;
  D6 = data.2;
  D5 = data.1;
  D4 = data.0;
  EN = 0;
  nop();
  EN = 1;
  delay(5);
}

void delay( char millisec)
/* 
  Delays a multiple of 1 milliseconds at 4 MHz (16F628 internal clock)
  using the TMR0 timer 
*/
{
    OPTION = 2;  /* prescaler divide by 8        */
    do  {
        TMR0 = 0;
        while ( TMR0 < 125)   /* 125 * 8 = 1000  */
            ;
    } while ( -- millisec > 0);
}

void init_serial( void )  /* initialise PIC16F690 bitbang serialcom */
{
   ANSEL.0 = 0; /* No AD on RA0             */
   ANSEL.1 = 0; /* No AD on RA1             */
   PORTA.0 = 1; /* marking line             */
   TRISA.0 = 0; /* output to PK2 UART-tool  */
   TRISA.1 = 1; /* input from PK2 UART-tool */
   receiver_flag = 0 ;
   return;      
}

void init_interrupt( void )
{
  IOCA.1 = 1; /* PORTA.1 interrupt on change */
  RABIE =1;   /* interrupt on change         */
  GIE = 1;    /* interrupt enable            */
  return;
}

void putchar( char ch )  /* sends one char */
{
  char bitCount, ti;
  PORTA.0 = 0; /* set startbit */
  for ( bitCount = 10; bitCount > 0 ; bitCount-- )
   {
     /* delay one bit 104 usec at 4 MHz       */
     /* 5+18*5-1+1+9=104 without optimization */ 
     ti = 18; do ; while( --ti > 0); nop(); 
     Carry = 1;     /* stopbit                    */
     ch = rr( ch ); /* Rotate Right through Carry */
     PORTA.0 = Carry;
   }
  return;
}

void printf(const char *string, char variable)
{
  char i, k, m, a, b;
  for(i = 0 ; ; i++)
   {
     k = string[i];
     if( k == '\0') break;   // at end of string
     if( k == '%')           // insert variable in string
      {
        i++;
        k = string[i];
        switch(k)
         {
           case 'd':         // %d  signed 8bit
             if( variable.7 ==1) putchar('-');
             else putchar(' ');
             if( variable > 127) variable = -variable;  // no break!
           case 'u':         // %u unsigned 8bit
             a = variable/100;
             putchar('0'+a); // print 100's
             b = variable%100;
             a = b/10;
             putchar('0'+a); // print 10's
             a = b%10;
             putchar('0'+a); // print 1's
             break;
           case 'b':         // %b BINARY 8bit
             for( m = 0 ; m < 8 ; m++ )
              {
                if (variable.7 == 1) putchar('1');
                else putchar('0');
                variable = rl(variable);
               }
              break;
           case 'c':         // %c  'char'
             putchar(variable);
             break;
           case '%':
             putchar('%');
             break;
           default:          // not implemented
             putchar('!');
         }
      }
      else putchar(k);
   }
}


/* *********************************** */
/*            HARDWARE                 */
/* *********************************** */

/*
         ___________  ___________
        |           \/           |
  +5V---|Vdd     16F690       Vss|---GND
        |RA5        RA0/AN0/(PGD)|
        |RA4            RA1/(PGC)|
        |RA3/!MCLR/(Vpp)  RA2/INT|
        |RC5/CCP              RC0|->-D4
        |RC4                  RC1|->-D5
  D7 -<-|RC3                  RC2|->-D6
        |RC6                  RB4|->- RS
        |RC7               RB5/Rx|
        |RB7/Tx               RB6|->- EN
        |________________________| 

*/

/*
           LCD Line length 16 (8+8) characters
           Internal ic: KS0066U		   
           _______________
          |               |
          |         Vss  1|--- GND  
          |         Vdd  2|--- +5V
          |    Kontrast  3|-<- Pot
          |          RS  4|-<- RB4
          |      RD/!WR  5|--- 0, GND
          |          EN  6|-<- RB6
          |          D0  7|
          |          D1  8|
          |          D2  9|
          |          D3 10|
          |          D4 11|-<- RC0
          |          D5 12|-<- RC1 
          |          D6 13|-<- RC2
          |          D7 14|-<- RC3 
          |_______________|						  
*/


