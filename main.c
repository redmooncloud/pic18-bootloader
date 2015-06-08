/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
    #include <xc.h>        /* XC8 General Include File */
#elif defined(HI_TECH_C)
    #include <htc.h>       /* HiTech General Include File */
#elif defined(__18CXX)
    #include <p18cxxx.h>   /* C18 General Include File */
#endif

#if defined(__XC) || defined(HI_TECH_C)

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#endif

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp */

#include <xc.h>
#include "p18f97j60.h"

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1L
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on SWDTEN bit))
#pragma config STVR = ON        // Stack Overflow/Underflow Reset Enable bit (Reset on stack overflow/underflow enabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG1H
#pragma config CP0 = OFF        // Code Protection bit (Program memory is not code-protected)

// CONFIG2L
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config FOSC2 = ON       // Default/Reset System Clock Select bit (Clock selected by FOSC1:FOSC0 as system clock is enabled when OSCCON<1:0> = 00)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Two-Speed Start-up (Internal/External Oscillator Switchover) Control bit (Two-Speed Start-up disabled)

// CONFIG2H
#pragma config WDTPS = 32768    // Watchdog Timer Postscaler Select bits (1:32768)

// CONFIG3L
#pragma config EASHFT = OFF     // External Address Bus Shift Enable bit (Address shifting disabled; address on external bus reflects the PC value)
#pragma config MODE = MM        // External Memory Bus (Microcontroller mode, external bus disabled)
#pragma config BW = 8           // Data Bus Width Select bit (8-Bit Data Width mode)
#pragma config WAIT = OFF       // External Bus Wait Enable bit (Wait states for operations on external memory bus disabled)

// CONFIG3H
#pragma config CCP2MX = ON      // ECCP2 MUX bit (ECCP2/P2A is multiplexed with RC1)
#pragma config ECCPMX = OFF     // ECCP MUX bit (ECCP1 outputs (P1B/P1C) are multiplexed with RH7 and RH6; ECCP3 outputs (P3B/P3C) are multiplexed with RH5 and RH4)
#pragma config ETHLED = OFF     // Ethernet LED Enable bit (RA0/RA1 function as I/O regardless of Ethernet module status)
void  uart_comm_process (uint8_t ecom );
void init_UART(void);
void init_timer( void );
#define ApplicationAddress 0x1000
typedef  void (*pFunction)(void);
pFunction Jump_To_Application,u;
uint32_t JumpAddress;
uint8_t getbyte ( char c );

unsigned int intr_cnt = 1;
unsigned int debug_cnt = 0;
unsigned int gest_tmr = 0;
unsigned int acty_tmr = 0;
uint8_t      uart_timeout;
uint8_t      uart_state;
uint8_t      c_buff[128],fa[64], u_buff[64], w_buff[16], r_buff[16];
volatile uint16_t  byte_counter;
uint8_t *buff,*ptr_b;
uint8_t g_flag;
uint32_t flash_addr;


/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/

/* i.e. uint8_t <variable_name>; */

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

void main(void)
{
    uint32_t r_addr;
    uint16_t addr,addr_hi,save_a, curr_a;
    uint8_t a,b,a_lo,a_hi,c,com,end_line,data, csum, pi;
    uint8_t num,num1,idx,idy,state,counter,com1,timeout,tx_r;
    uint8_t b_line;
    char ch;
    /*p = (uint32_t *)0xC00;
    u = &init_UART;
    *p = u;
    p = (uint32_t *)0xE00;
    *p = 0x12345678;
    val = *p;*/
    // if ( val == 0xdeaddead) { while ( 1 ) ; }
    
    INTCONbits.GIE = 0;

    // a = RCREG1;
    TRISJbits.TRISJ5 = 0;   // Output - Status green LED
    LATJbits.LATJ5 = 0; 
    
     TRISJbits.TRISJ0 = 0;   // Output - Status yellow LED high lights LED
    LATJbits.LATJ0 = 0;
    
     TRISJbits.TRISJ4 = 0;   // Output - Status yellow LED high lights LED
    LATJbits.LATJ4 = 0;
    
    TRISG = 0xF5;           // RG0,RG2,RG4,RG5,RG6,RG7 input..RG1,RG3 output
    LATG = 0x0A;            // M_RST=Act_low; ?keyboard lockout levels
    TRISGbits.TRISG3 = 0;   // Keyboard lockout - output
    LATGbits.LATG3 = 1;     // Set keyboard lockout high

    
    ReadFlash(0x1000, 64, fa);
    if ( fa[8] == 0xff && fa[9] == 0xff && fa[24] == 0xff && fa[25] == 0xff ) goto M1;
    
    if ( (PORTG & 0xF0) == 0 ) goto M0;
    
    if ( *(uint16_t *)0xE00 != 0xdead )
    { /* Test if user code is programmed starting from address 0x8003000 */
   
      //JumpAddress = *(uint32_t*) (ApplicationAddress + 4);
      //Jump_To_Application = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      // ReadFlash(r_addr, 64, fa);  
      STKPTR = 0;
      asm("GOTO 0x1000");
    
    }
    M0:
    //EraseFlash(0x1000,0x1f000);
    M1:
    init_UART();
    init_timer();
    a = RCREG1;
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    // EraseFlash(0x1000,0x1f000);
    InitApp();
    idx = 0;
    state = 0;
    com1 = 0;
    com = 0;
    end_line = 0;
    r_addr = 0;
    addr_hi = 0;
    pi = 0;
    ptr_b = &c_buff[0];
    /* TODO <INSERT USER APPLICATION CODE HERE> */

    do {
 
     do { 

     if ( PIR1bits.RC1IF == 1 )  {        
         ch = RCREG1;
         timeout = 0;
         break;
     }

     if(PIR1bits.TMR1IF) {
     TMR1 = 0xE795;
     PIR1bits.TMR1IF = 0;
     counter++;
     if ( state ) timeout++;
     if ( timeout > 200 ) {
         idx = 0;
         pi = 0;
         state = 0;
         com1 = 0;
         com = 0;
         end_line = 0;
         r_addr = 0;
         addr_hi = 0;
         timeout = 0;
         
     }
     if ( counter == 20 ) { LATJbits.LATJ0 = 1;   LATJbits.LATJ4 = 0; }
     else if ( counter == 40 ) {  LATJbits.LATJ0 = 0; LATJbits.LATJ4 = 1; counter = 0; } 

     }

     } while ( 1 );

     switch ( state ) {

     case 0:
     if ( ch == ':' ) { state = 1; b_line = 0; csum = 0;}
     else if ( ch == '#' ) { state = 9; }
     else {
         while ( PIR1bits.TX1IF == 0 );
         TXREG1 = 0x15;
     }
     break;

     case 1:
     a = getbyte( ch ); 
     state = 2;
     break;


     case 2:
     b = getbyte( ch ); 
     num = a<<4 | b; num1 = num;
     csum += num;
     if ( num % 16 != 0 ) b_line = 1;
     else b_line = 0; 
     state = 3;
     break;

     case 3:
     a = getbyte( ch ); 
     state = 4;
     break;

     case 4:
     b = getbyte( ch ); 
     a_hi = a<<4 | b;
     csum += a_hi;
     state = 5;
     break;

     case 5:
     a = getbyte( ch ); 
     state = 6;
     break;

     case 6:
     b = getbyte( ch ); 
     a_lo = a<<4 | b;
     csum += a_lo;
     addr = a_hi<<8 | a_lo;
     //r_addr = addr_hi<<16 | addr;
     state = 7;
     break;

     case 7:
     a = getbyte( ch ); 
     state = 8;
     break;

     case 8:
     b = getbyte( ch ); 
     com = a<<4 | b;
     csum += com;
     if ( com == 0 ) {
         if( num ) state = 12; 
         else state = 14;
         if ( idx == 0 ) {
         r_addr = addr_hi<<16 | addr;
         //ptr_b = &c_buff[pi];
         save_a = addr & 0xffc0;
     }
      curr_a = addr & 0xffc0;
      if (save_a != curr_a ){
         Nop();
         pi ^= 0x40;
       }
      // pi ^= 0x40;
     
         ptr_b = &c_buff[pi];
         //buff = &c_buff[0];
         buff = ptr_b;
     }else if ( com == 1 ) { 
         if ( num) { state = 12;
         buff = &r_buff[0];} 
         else state = 14;
     } else if ( com == 4 ) {
         state = 12;
         buff = &u_buff[0];
     }
     else state = 0;
     break;

     case 9:
     num = ch;
     idy = 0;
     state = 10;
     break;
     
     case 10:
     w_buff[idy++] = ch;
     num--;
     if ( num == 0) {
     uart_comm_process( w_buff[0] );
     state = 0;
     }
     break;

     case 12:
     a = getbyte( ch ); 
     state = 13;
     break;

     case 13:
     b = getbyte( ch ); 
     data = a<<4 | b;
     buff[idx++] = data;
     csum += data;
      if ( idx >= 64 ) {
         Nop();
         idx = 0;
     } 
     num--;
     if ( num )
     {
         state = 12;
     }
     else
     {
         state = 14;
     }
     break;

     case 14:
     a = getbyte( ch ); 
     state = 15;
     break;

     case 15:
     b = getbyte( ch ); 
     c = a<<4 | b;
     if ( ((0xFF - csum + 1) == c ) || ( csum == 0 )) {
         tx_r = 0x06; 
         state = 16;
         if ( com == 4 ) {
             a = u_buff[0];
             b = u_buff[1];
             addr_hi = a<<8 | b;
         }
     }
     else { tx_r = 0x15;state = 18; }
     break;

     case 16:
     if (ch  ==  10){ 
         //TXREG1 = 0x06;
         state = 0; 
         end_line = 1;
         if ( com == 1)  { com1 = 1; }
     }
     else  { 
         state = 17;                     // probably ch == 13 
     }     
     break;


     case 17:
       Nop();
       //TXREG1 = 0x06;
       state = 0;
       end_line = 1;
       if ( com == 1) { com1 = 1; }
     break;
     
     
     case 18:
     if (ch  ==  10){ 
         end_line = 2;
         state = 0; 
     }
     else  { 
         state = 19; 
     }     
     break;


     case 19:     
     //TXREG = 0x15;
     end_line = 2;
     state = 0;
     break;
     

     default:
     TXREG1 = 0x15;
     state = 0;
     break;
     } 
     
   //  while ( PIR1bits.TX1IF == 0 );
     if ( end_line == 1 ) {
         if ( ((idx == 0) && ( com == 0 ))  ) {
             Nop();
             Nop();
             WriteBlockFlash(r_addr,1,ptr_b);
         } else if ( com == 4 ) {
             WriteBlockFlash(r_addr,1,ptr_b);
             idx = 0;
         }
         // WriteBytesFlash(r_addr, num1, c_buff); 
         end_line = 0;
         while ( PIR1bits.TX1IF == 0 );
         TXREG1 = tx_r;
         
     }
     if ( com1 == 1) {
      while ( PIR1bits.TX1IF == 0 );
      TXREG1 = tx_r;
      while ( PIR1bits.TX1IF == 0 );
      Nop();
      idx = 0;
      pi = 0;
      state = 0;
      com1 = 0;
       com = 0;
       end_line = 0;
       r_addr = 0;
       addr_hi = 0;
     // *(uint16_t *)0xE00 = 0xdead;
      // asm("reset");
     }
     
    //while ( PIR1bits.TX1IF == 0 );
    } while ( 1 );


}



void init_timer( void )
{
        TMR1H = 0xE7; //1ms interrupt
        TMR1L = 0x95;
        
        PIE1bits.TMR1IE = 0;          //disable interrupt
        T1CONbits.TMR1ON = 1;           //timer1 = on if set. bit 0
        T1CONbits.TMR1CS = 0;           //take clock source from Fosc/4 when 0. if =1, then external oscilator bit 1
        T1CONbits.T1SYNC = 1;           //set = do not synchronize bit 2
        T1CONbits.T1OSCEN = 0;          //set = oscillator enabled bit 3
        T1CONbits.T1CKPS0 = 1;          //prescale = 0 bit 4
        T1CONbits.T1CKPS1 = 1;          //prescale = 0 bit 5
        T1CONbits.T1RUN = 0;            //Device clock derived from another source
        T1CONbits.RD16 = 1;             //Two 8 bit R/W operations.
        IPR1bits.TMR1IP = 0; 
}

void init_UART(void)
{
    
    TRISC1 = 1;
    TRISC6 = 0;
    TRISC7 = 1;
    SPBRG1 = ((_XTAL_FREQ/16)/BAUD)-1;
    TXSTA1bits.BRGH = 1;    //Fast baud rate
    BAUDCON1bits.BRG16 = 0; // 8-bit baud rate generator
    TXSTA1bits.SYNC1 = 0;   // Asynchronous mode

    RCSTA1bits.SPEN = 1;    // Serial port enable bit
    RCSTA1bits.RX9  = 0;    // 8-bit reception
    RCSTA1bits.CREN = 1;

    EIEbits.TXIE = 0;       // Disable transmit interrupt enable
    TXSTA1bits.TX9 = 0;     // 8 data bits
    TXSTA1bits.TXEN = 0;    // Reset transmitter
    TXSTA1bits.TXEN = 1;    // Enable transmitter

    
    
    
}


uint8_t getbyte ( char c )
{
        uint8_t ret;
        if ( c >= 'A' ) { ret = c - 'A' + 10; }
        else { ret =  c - '0'; }
        return ret;
}



void  uart_comm_process (uint8_t ecom )
{
        volatile uint8_t         a,b,*p;
        volatile uint16_t        m,mb,*ma;
        uint8_t  *buff,rb,uf[64],i;
        uint16_t byte_counter;
        uint32_t a1, a2;

        

        switch( (uint8_t)(ecom)  ) {

        case 1:
        TXREG1  = **((uint8_t **)&w_buff[1] );                 /*read 1 byte data mem */
        return;

        case 2:
        buff  = *((uint8_t **)&w_buff[1] );
        byte_counter = *((uint16_t *)&w_buff[3] );
        if ( byte_counter == 0 ) return;
        do {
        while ( PIR1bits.TX1IF == 0 );
        TXREG1 = *buff++;
        }   while (--byte_counter);     /* read block bytes data mem */
        return;

        case 3:
        flash_addr  = *((uint32_t *)&w_buff[1] );
        byte_counter = *((uint16_t *)&w_buff[5] );        /* read block flash bytes programm memory */
        if ( byte_counter == 0 ) return;
        
        do {
        if ( byte_counter > 64 ) rb = 64;else rb = byte_counter;
        byte_counter -= rb;
        ReadFlash(flash_addr, rb, uf);
        flash_addr += rb;
        i = 0;
        do {  
        while ( PIR1bits.TX1IF == 0 );
        TXREG1 = uf[i];
        i++;
        }   while (--rb);
        } while ( byte_counter);
        return;

        case 4:
        p =    *((uint8_t **)&w_buff[1]);
        a =    *((uint8_t *)&w_buff[3] );
        *p = a;
        //**((volatile uint8_t **)&area[1]) = *((volatile uint8_t *)&area[3] );
        while ( PIR1bits.TX1IF == 0 );
        TXREG1  = **((uint8_t **)&w_buff[1]);
        return;

        case 5:
        ma =   *((uint16_t **)&w_buff[1]);
        mb =   *((uint16_t *)&w_buff[3]);
        *ma = mb;
        //(**((volatile uint16_t **)&area[1])) = *((volatile uint16_t *)&area[3] ); /* write 1 word  to data memory */
        buff  = *((uint8_t **)&w_buff[1] );
        byte_counter = 2;
        do {
        while ( PIR1bits.TX1IF == 0 );
        TXREG1 = *buff++;
        }   while (--byte_counter); 
        return;
        
            case 17:
                a = *((uint8_t *)&w_buff[1] );
                b = *((uint8_t *)&w_buff[2] );
                if ( a < 1  ) a = 1; if ( a > 31 ) a = 31;
                if ( b > 31 ) b = 31; if ( b < 1 ) b = 1;
                a1 = a * 4096; a1 = a1 & 0x1FFFF;
                a2 = b * 4096; a2 = a2 & 0x1FFFF;
                if ( b > a)
                    EraseFlash(a1, a2);
                else 
                    EraseFlash(a2, a1);
                return;

            case 20:
                *(uint16_t *)0xe00 = 0xdead;
                asm("reset");
                return;

            case 21:
                *(uint16_t *)0xe00 = 0;
                asm("reset");
                return;

        default:
        return;
        }
}



void ReadFlash(unsigned long startaddr, unsigned int num_bytes, unsigned char *flash_array)
{
	DWORD_VAL flash_addr;


	flash_addr.Val = startaddr;

	TBLPTRU = flash_addr.byte.UB; //Load the address to Address pointer registers
	TBLPTRH = flash_addr.byte.HB;	
	TBLPTRL	= flash_addr.byte.LB;

	while(num_bytes--)
	{
		//*********** Table read sequence ******************************

		asm("TBLRDPOSTINC");
		*flash_array++ = TABLAT;
        // asm("TBLRDPOSTINC");
	}	

}
