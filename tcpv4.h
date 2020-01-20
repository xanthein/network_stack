#ifndef TCPV4_H
#define TCPV4_H

#include <stdint.h>

struct tcpv4_header {
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t seq_number;
	uint32_t ack_number;
	uint8_t ns : 1;
	uint8_t reserved : 3;
	uint8_t data_offset : 4;
	uint8_t fin : 1;
	uint8_t syn : 1;
	uint8_t rst : 1;
	uint8_t psh : 1;
	uint8_t ack : 1;
	uint8_t urg : 1;
	uint8_t ece : 1;
	uint8_t cwr : 1;
	uint16_t window_size;
	uint16_t checksum;
	uint16_t urgent_pointer;
	uint8_t data[];
} __attribute__((packed));

#define TCPV4_HEADER_SIZE sizeof(struct tcpv4_header)

#define FLAG_FIN (1<<0)
#define FLAG_SYN (1<<1)
#define FLAG_RST (1<<2)
#define FLAG_PSH (1<<3)
#define FLAG_ACK (1<<4)
#define FLAG_URG (1<<5)

typedef enum {
	TCPV4_CLOSED,
	TCPV4_LISTEN,
	TCPV4_SYN_SENT,
	TCPV4_SYN_RCVD,
	TCPV4_ESTABLISHED,
	TCPV4_FIN_WAIT_1,
	TCPV4_FIN_WAIT_2,
	TCPV4_CLOSE_WAIT,
	TCPV4_CLOSING,
	TCPV4_LAST_ACK,
	TCPV4_TIME_WAIT,
} tcpv4_states;

typedef enum {
	NONE = 0,
	PASSIVE_OPEN,
	ACTIVE_OPEN,
	CLOSE,
	RECV_SYN,
	RECV_ACK,
	RECV_SYN_ACK,
	RECV_FIN,
	RECV_FIN_ACK,
	RECV_RST,
	RECV_RST_ACK,
	RECV_PSH,
	RECV_PSH_ACK,
	TIMEOUT
} tcpv4_events;

typedef enum {
	EOL = 0,
	NOP,
	MSS,
} tcpv4_options;

struct tcpv4_opt_mss {
	uint8_t kind;
	uint8_t length;
	uint16_t mss;
};

struct tcp_control {
	uint16_t local_port;
	uint16_t dst_port;
	uint32_t dst_ip;

	uint32_t local_seq;
	uint32_t local_ack;
	uint16_t local_window;

	uint32_t remote_seq;
	uint32_t remote_next_seq;
	uint16_t remote_window;

	tcpv4_states state;
	tcpv4_events event;
	
	uint8_t *rx_buffer;
	uint8_t *rx_buffer_ptr;
	uint8_t *tx_buffer;
	uint8_t *tx_buffer_ptr;
	
//應用層輸出佇列
//tcp的重傳
	uint16_t mss;
//iss
	struct tcp_control *prev;
	struct tcp_control *next;
};

int tcpv4_listen(uint16_t port);
int tcpv4_close(uint16_t port);
void tcpv4_init();
void tcpv4_exit();
int tcpv4_read(uint8_t *buffer, uint32_t size);
int tcpv4_write(uint8_t *buffer, uint32_t size, struct tcp_control *tcb, uint8_t flags);

#endif
