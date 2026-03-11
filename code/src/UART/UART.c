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
	for (char i = 7; i >= 0; i--)
		UART_tx(byte & 1 << i);
}

// Non-recursive version to reduce stack usage
void	UART_print_num(const uint32_t number)
{
	char buffer[11]; // Max uint32_t is 10 digits + null terminator
	uint8_t i = 0;
	uint32_t n = number;
	
	// Handle zero case
	if (n == 0) {
		UART_tx('0');
		return;
	}
	
	// Build digits in reverse order
	while (n > 0) {
		buffer[i++] = '0' + (n % 10);
		n /= 10;
	}
	
	// Print in correct order
	while (i > 0) {
		UART_tx(buffer[--i]);
	}
}
