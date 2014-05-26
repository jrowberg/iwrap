#ifndef _UART_H_
#define _UART_H_

int uart_open(char *port);
void uart_close();
int uart_tx(int len, unsigned char *data);
int uart_rx(int len, unsigned char *data, int timeout_ms);

#endif // _UART_H_