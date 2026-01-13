#include "UART.h"

void UART_init(void) {
  unsigned int ubrr = MYUBRR;

  UCSR0A = (1 << U2X0);
  UBRR0L = (unsigned char)ubrr;
  UCSR0B |= (1 << TXEN0) | 1 << RXEN0;
  UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_tx(char c) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = c;
}

uint8_t	UART_rx(void)
{
	while (!(UCSR0A & 1 << RXC0));
	return (UDR0);
}

void UART_print_str(char *str) {
  while(*str) UART_tx(*str++);
}

void UART_print_hex(uint8_t hex) {
  const char hex_chars[] = "0123456789ABCDEF";
  UART_tx(hex_chars[hex >> 4]);
  UART_tx(hex_chars[hex & 0x0F]);
}
void	UART_print_byte(const uint8_t byte)
{
	for (unsigned char i = 7; i >= 0; i--)
		UART_tx(byte & 1 << i);
}

void	UART_print_num(const uint16_t number)
{
	const uint8_t num = number % 10;
	if (number > 9)
		UART_print_num(number / 10);
	UART_tx(num + '0');
}
