#ifndef PACKETS_H
#define PACKETS_H

#include <stdint.h>

#define PACKET_BUF_SIZE 0x8000

static const char INTERRUPT_CHAR = '\x03';

uint8_t *inbuf_get();
int inbuf_end();
void inbuf_erase_head(ssize_t end);
void write_flush();
void write_packet(const char *data);
void write_binary_packet(const char *pfx, const uint8_t *data, ssize_t num_bytes);
void read_packet();
bool try_input_interrupt(void);
void remote_prepare(char *name);

#endif /* PACKETS_H */
