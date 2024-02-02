/************************************************************
 *   Author : German Mundinger
 *   Date   : 2021
 ************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

// TIMER1
#define DDR_TIMER1      DDRB
#define PORT_TIMER1     PORTB
#define PIN_TIMER1_OC1A PORTB1
#define PIN_TIMER1_OC1B PORTB2

// I2C
#define DDR_I2C		DDRC
#define PORT_I2C	PORTC
#define PIN_I2C_SDA	PORTC4
#define PIN_I2C_SCL	PORTC5

// SPI
#define DDR_SPI     DDRB
#define PORT_SPI    PORTB
#define PIN_MOSI    PORTB3
#define PIN_MISO    PORTB4
#define PIN_SCK     PORTB5
#define PIN_SS      PORTB2

// UART
//#define UART_RX_ENABLE
#define UART_RX_BUFFER_SIZE	64U

// INT0
#define DDR_INT0    DDRD
#define DDR_N_INT0  PD2
#define PORT_INT0   PORTD
#define PORT_N_INT0 PORTD2

// INT1
#define DDR_INT1    DDRD
#define DDR_N_INT1  PD3
#define PORT_INT1   PORTD
#define PORT_N_INT1 PORTD3

// PCINT0
#define DDR_PCINT_0     DDRB
#define DDR_N_PCINT_0   PB0
#define PORT_PCINT_0    PORTB
#define PORT_N_PCINT_0  PORTB0
#define PIN_PCINT_0     PINB
#define PIN_N_PCINT_0   PINB0

// PCINT11
#define DDR_PCINT_11    DDRC
#define DDR_N_PCINT_11  PC3
#define PORT_PCINT_11   PORTC
#define PORT_N_PCINT_11 PORTC3
#define PIN_PCINT_11    PINC
#define PIN_N_PCINT_11  PINC3

// PC_INT_D
#define DDR_PCINT_D     DDRD
#define PORT_PCINT_D    PORTD
#define PIN_PCINT_D     PIND
#define PORT_N_PCINT_16 PORTD0
#define PIN_N_PCINT_16  PIND0
#define PORT_N_PCINT_21 PORTD5
#define PIN_N_PCINT_21  PIND5

#endif // CONFIG_H

