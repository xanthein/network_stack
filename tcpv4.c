#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "ethernet.h"
#include "ipv4.h"
#include "tcpv4.h"
#include "utils.h"

#define MAX_SEGMENT_SIZE 1460
#define RECEIVE_BUFFER_SIZE 65535

struct tcp_control *tcp_control_list;
uint8_t *write_buffer;

uint32_t generate_sequence()
{
	return 12345;
}

struct tcp_control *look_for_control(uint16_t dst_port)
{
	struct tcp_control *tcb = tcp_control_list;
	
	while(tcb != NULL) {
		if(tcb->local_port == dst_port)
			return tcb;
		tcb = tcb->next;
	}
	return NULL;
}

void insert_control(struct tcp_control *tcb)
{
	if(tcp_control_list != NULL) {
		tcb->next = tcp_control_list;
		tcp_control_list->prev = tcb;
	}
	tcp_control_list = tcb;
	tcb->prev = NULL;
}

void delete_control(struct tcp_control *tcb)
{
	tcb->next->prev = tcb->prev;
	tcb->prev->next = tcb->next;

	free(tcb->rx_buffer);
	free(tcb);
}

struct tcp_control* alloc_control()
{
	struct tcp_control *tcb;
	
	tcb = (struct tcp_control *)calloc(1, sizeof(struct tcp_control));
	if(tcb == NULL) {
		printf("can't alloc tcb\n");
		return NULL;
	}
	return tcb;
}

int is_seq_valid(struct tcp_control *tcb, struct tcpv4_header *header, uint32_t data_size)
{
	//don't have seq number to check with
	if(tcb->state == TCPV4_CLOSED ||
		tcb->state == TCPV4_LISTEN ||
		tcb->state == TCPV4_SYN_SENT)
		return 1;

	if(data_size > 0 && (tcb->local_window == 0))
		return 0;

	if(header->seq_number < tcb->remote_next_seq ||
		header->seq_number > tcb->remote_next_seq + tcb->local_window)
		return 0;

	return 1;
}

uint32_t save_data(struct tcp_control *tcb, void *data, uint32_t data_size)
{
	uint32_t buffer_size;

	if(tcb->local_window > data_size)
		buffer_size = data_size;
	else
		buffer_size = tcb->local_window;

	if((tcb->rx_buffer_w_ptr + buffer_size) > (tcb->rx_buffer + RECEIVE_BUFFER_SIZE)) {
		uint32_t exceed_size = (tcb->rx_buffer_w_ptr + buffer_size) -
			(tcb->rx_buffer + RECEIVE_BUFFER_SIZE);
		memcpy(tcb->rx_buffer_w_ptr, data, (buffer_size - exceed_size));
		memcpy(tcb->rx_buffer, data+(buffer_size - exceed_size), exceed_size);
		tcb->rx_buffer_w_ptr = tcb->rx_buffer + exceed_size;
	}
	else {
		memcpy(tcb->rx_buffer_w_ptr, data, buffer_size);
		tcb->rx_buffer_w_ptr += buffer_size;
	}

	tcb->local_window = tcb->local_window - buffer_size;

	return buffer_size;
}

void update_state_machine(struct tcp_control *tcb, struct tcpv4_header *header,
		uint32_t packet_size, uint32_t remote_address)
{
	switch(tcb->state) {
		case TCPV4_CLOSED:
			switch(tcb->event) {
				case PASSIVE_OPEN:
					tcb->local_seq = generate_sequence();
					tcb->state = TCPV4_LISTEN;
					break;
				case ACTIVE_OPEN:
					//TODO set timeout
					tcb->local_seq = generate_sequence();
					tcpv4_write(NULL, 0, tcb, FLAG_SYN);
					tcb->state = TCPV4_SYN_SENT;
					break;
			}
			break;
		case TCPV4_LISTEN:
			switch(tcb->event) {
				case CLOSE:
					delete_control(tcb);
					break;
				case RECV_SYN:
					tcb->dst_ip = remote_address;
					tcb->dst_port = header->src_port;
					tcb->remote_seq = header->seq_number;
					tcb->remote_next_seq = header->seq_number + 1;
					tcb->remote_window = header->window_size;
					tcb->local_ack = 0;
					//TODO set initial timeout
					tcpv4_write(NULL, 0, tcb, FLAG_SYN | FLAG_ACK);
					tcb->state = TCPV4_SYN_RCVD;
					break;
			}
			break;
		case TCPV4_SYN_SENT:
			//TODO handle event at SYN_SENT state
			switch(tcb->event) {
				case RECV_ACK:
					break;
				case RECV_SYN:
					break;
				case RECV_SYN_ACK:
					break;
			}
			break;
		case TCPV4_SYN_RCVD:
			switch(tcb->event) {
				case RECV_SYN_ACK:
					//TODO handle simultaneous open
					break;
				case RECV_ACK:
					if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
						if(tcb->remote_next_seq == header->seq_number) {
							if((tcb->local_seq+1) == header->ack_number) {
								tcb->local_seq+=1;
								//TODO reset timer
								tcb->state = TCPV4_ESTABLISHED;
							}
						}
					}
					break;
				case RECV_FIN_ACK:
					if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
						if(tcb->remote_next_seq == header->seq_number) {
							if((tcb->local_seq+1) == header->ack_number) {
								tcpv4_write(NULL, 0, tcb, FLAG_ACK);
								tcb->state = TCPV4_CLOSE_WAIT;
							}
						}
					}
					break;
				case CLOSE:
					tcpv4_write(NULL, 0, tcb, FLAG_FIN);
					tcb->state = TCPV4_FIN_WAIT_1;
					break;
			}
			break;
		case TCPV4_ESTABLISHED:
			switch(tcb->event) {
				case RECV_PSH_ACK:
				case RECV_ACK:
					printf("tcb->local_seq = %u\n", tcb->local_seq);
					printf("tcb->local_ack = %u\n", tcb->local_ack);
					printf("tcb->remote_seq = %u\n", tcb->remote_seq);
					printf("tcb->remote_next_seq = %u\n", tcb->remote_next_seq);
					printf("header->seq_number = %u\n", header->seq_number);
					printf("header->ack_number = %u\n", header->ack_number);
					if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
						if(tcb->remote_next_seq == header->seq_number) {
							if(tcb->local_ack < header->ack_number && 
								header->ack_number <= (tcb->local_seq+1)) {
								//TODO check data left for sending
								//receive data
								if(packet_size > 0) {
									tcb->remote_seq = header->seq_number;
									uint32_t data_write = save_data(tcb, header->data, packet_size - TCPV4_HEADER_SIZE);
									tcb->remote_next_seq = tcb->remote_seq + data_write;
									tcpv4_write(NULL, 0, tcb, FLAG_ACK);
								}
							}
						}
					}
					break;
				case RECV_FIN_ACK:
					if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
						if(tcb->remote_next_seq == header->seq_number) {
							if(tcb->local_ack < header->ack_number &&
								header->ack_number <= (tcb->local_seq+1)) {
								tcb->remote_seq = header->seq_number;
								if(packet_size > 0) {
									uint32_t data_write = save_data(tcb, header->data, packet_size - TCPV4_HEADER_SIZE);
									tcb->remote_next_seq = tcb->remote_seq + data_write;
								} else
									tcb->remote_next_seq = header->seq_number + 1;
								tcpv4_write(NULL, 0, tcb, FLAG_ACK);
								tcb->state = TCPV4_CLOSE_WAIT;
							}
						}
					}
					break;
				case CLOSE:
					tcpv4_write(NULL, 0, tcb, FLAG_FIN);
					tcb->state = TCPV4_FIN_WAIT_1;
					break;
			}
			break;
		case TCPV4_FIN_WAIT_1:
			switch(tcb->event) {
				case RECV_ACK:
					if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
						if(tcb->remote_next_seq == header->seq_number) {
							if(tcb->local_ack < header->ack_number &&
								header->ack_number <= (tcb->local_seq+1)) {
								tcb->remote_seq = header->seq_number;
								tcb->remote_next_seq = header->seq_number + 1;
								tcb->state = TCPV4_FIN_WAIT_2;
							}
						}
					}
					break;
				case RECV_FIN_ACK:
					if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
						if(tcb->remote_next_seq == header->seq_number) {
							if(tcb->local_ack < header->ack_number &&
								header->ack_number <= (tcb->local_seq+1)) {
								tcb->remote_seq = header->seq_number;
								tcb->remote_next_seq = header->seq_number + 1;
								tcb->state = TCPV4_CLOSING;
							}
						}
					}
					break;
			}
			break;
		case TCPV4_FIN_WAIT_2:
			if(tcb->event == RECV_FIN_ACK) {
				if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
					if(tcb->remote_next_seq == header->seq_number) {
						if(tcb->local_ack < header->ack_number &&
							header->ack_number <= (tcb->local_seq+1)) {
							tcb->remote_seq = header->seq_number;
							tcb->remote_next_seq = header->seq_number + 1;
							tcb->state = TCPV4_TIME_WAIT;
							tcpv4_write(NULL, 0, tcb, FLAG_ACK);
						}
					}
				}
			}
			break;
		case TCPV4_CLOSE_WAIT:
			if(tcb->event == CLOSE) {
				tcpv4_write(NULL, 0, tcb, FLAG_FIN);
				tcb->state = TCPV4_FIN_WAIT_1;
			}
			break;
		case TCPV4_CLOSING:
			if(tcb->event == RECV_ACK) {
				if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
					if(tcb->remote_next_seq == header->seq_number) {
						if(tcb->local_ack < header->ack_number &&
							header->ack_number <= (tcb->local_seq+1)) {
							tcb->state = TCPV4_TIME_WAIT;
						}
					}
				}
			}
		case TCPV4_LAST_ACK:
			if(tcb->event == RECV_ACK) {
				if(tcb->dst_ip == remote_address && tcb->dst_port == header->src_port) {
					if(tcb->remote_next_seq == header->seq_number) {
						if(tcb->local_ack < header->ack_number &&
							header->ack_number <= (tcb->local_seq+1)) {
							tcb->state = TCPV4_CLOSED;
						}
					}
				}
			}
			break;
		case TCPV4_TIME_WAIT:
			break;
	}
}

int tcpv4_connect()
{
}

int tcpv4_listen(uint16_t port)
{
	struct tcp_control *tcb;

	tcb = look_for_control(port);
	if(tcb) {
		printf("port already been listend\n");
		return -1;
	}
	
	tcb = alloc_control();
	if(tcb == NULL) {
		printf("alloc_control failed\n");
		return -1;
	}
	tcb->local_port = port;
	tcb->event = PASSIVE_OPEN;
	tcb->local_window = RECEIVE_BUFFER_SIZE;
	tcb->rx_buffer = calloc(1, RECEIVE_BUFFER_SIZE);
	tcb->rx_buffer_w_ptr = tcb->rx_buffer;
	tcb->rx_buffer_r_ptr = tcb->rx_buffer;
	insert_control(tcb);

	update_state_machine(tcb, NULL, 0, 0);

	return 0;
}

int tcpv4_timeout()
{
}

int tcpv4_close(uint16_t port)
{
	struct tcp_control *tcb;

	tcb = look_for_control(port);
	if(tcb == NULL) {
		printf("port not used\n");
		return -1;
	}

	tcb->event = CLOSE;

	update_state_machine(tcb, NULL, 0, 0);

	return 0;
}

void tcpv4_parse_flag_option(struct tcp_control *tcb, struct tcpv4_header *header, uint32_t size)
{
	uint8_t option_len = header->data_offset*4 - TCPV4_HEADER_SIZE;
	uint8_t *option = header->data;
	struct tcpv4_opt_mss *opt_mss;

	//parse flag
	if(header->syn)
		tcb->event = header->ack?RECV_SYN_ACK:RECV_SYN;
	else if(header->fin)
		tcb->event = header->ack?RECV_FIN_ACK:RECV_FIN;
	else if(header->rst)
		tcb->event = header->ack?RECV_RST_ACK:RECV_RST;
	else if(header->psh)
		tcb->event = header->ack?RECV_PSH_ACK:RECV_PSH;
	else if(header->ack)
		tcb->event = RECV_ACK;

	//parse option
	while(option_len > 0) {
		switch(*option) {
			case NOP://length 1
				option++;
				option_len--;
				break;
			case MSS://length 4
				opt_mss = (struct tcpv4_opt_mss *)option;
				tcb->mss = ntohs(opt_mss->mss);
				option+=4;
				option_len-=4;
				break;
			default:
				printf("Not support opt %d\n", *option);
				uint8_t len = *(option+1);
				option+=len;
				option_len-=len;
				break;
		}
	}
}

int tcpv4_read(uint8_t *buffer, uint32_t size)
{
	struct ether_frame *ether = (struct ether_frame *)buffer;
	struct ipv4_header *ip = (struct ipv4_header *)(ether->data);
	struct tcpv4_header *header = (struct tcpv4_header *)(ip->data);
	struct tcp_control *tcb;
	uint32_t tcp_packet_length = ip->length - IPV4_HEADER_SIZE; 
	uint16_t checksum;

	checksum = calculate_tcp_checksum((uint8_t *)header, tcp_packet_length,
			htonl(ip->src_ip), htonl(ip->dst_ip), htons(tcp_packet_length));

	if(checksum) {
		printf("tcpv4 checksum error\n");
		free(buffer);
		return -1;
	}

	header->src_port = ntohs(header->src_port);
	header->dst_port = ntohs(header->dst_port);
	header->seq_number = ntohl(header->seq_number);
	header->ack_number = ntohl(header->ack_number);
	header->window_size = ntohs(header->window_size);

	tcb = look_for_control(header->dst_port);
	if(tcb) {
		if(is_seq_valid(tcb, header, tcp_packet_length)) {
			tcpv4_parse_flag_option(tcb, header, ip->length - IPV4_HEADER_SIZE);
			update_state_machine(tcb, header, tcp_packet_length, ip->src_ip);
		}
	}
	else {
		printf("no transmission control block for packet\n");
	}

	free(buffer);
	return 0;
}

//TODO add support to more option kind
int tcpv4_write(uint8_t *buffer, uint32_t size, struct tcp_control *tcb, uint8_t flags)
{
	struct ether_frame *ether = (struct ether_frame *)write_buffer;
	struct ipv4_header *ip = (struct ipv4_header *)(ether->data);
	struct tcpv4_header *header = (struct tcpv4_header *)(ip->data);
	struct tcpv4_opt_mss *opt_mss = (struct tcpv4_opt_mss *)(header->data);
	uint16_t checksum;
	uint32_t data_length, tcpv4_length, packet_length;
	
	data_length = ((flags & FLAG_SYN) | (flags & FLAG_RST))?0:size;
	//TODO adjust data_length according to remote window and mss
	tcpv4_length = TCPV4_HEADER_SIZE + ((flags & FLAG_SYN)?4:0) + data_length;
	packet_length = ETH_HEADER_SIZE + IPV4_HEADER_SIZE + tcpv4_length;

	memset(ether, 0, packet_length);

	if(data_length)
		memcpy(header->data, buffer, size);

	header->src_port = htons(tcb->local_port);
	header->dst_port = htons(tcb->dst_port);
	header->seq_number = htonl(tcb->local_seq);
	header->ack_number = htonl(tcb->remote_next_seq);
	header->window_size = htons(tcb->local_window);
	header->data_offset = tcpv4_length/4;

	header->fin = (flags & FLAG_FIN)?1:0;
	header->syn = (flags & FLAG_SYN)?1:0;
	header->rst = (flags & FLAG_RST)?1:0;
	header->psh = (flags & FLAG_PSH)?1:0;
	header->ack = (flags & FLAG_ACK)?1:0;
	header->urg = (flags & FLAG_URG)?1:0;

	if(flags & FLAG_SYN) {
		opt_mss->kind = MSS;
		opt_mss->length = 4;
		opt_mss->mss = htons(tcb->mss);
	}

	header->checksum = calculate_tcp_checksum((uint8_t *)header, tcpv4_length, 
			htonl(ipv4_info.ip), htonl(tcb->dst_ip), htons(tcpv4_length));

	//TODO add retransmite
	if(ipv4_write((uint8_t *)ether, packet_length, tcb->dst_ip, IP_TCP) > 0) {
		tcb->local_seq += data_length;
		return 0;
	}
	else {
		return -1;
	}
}

int tcpv4_collect_data(uint16_t port, uint8_t *buffer, uint32_t size)
{
	uint16_t data_size, read_data_size;
	struct tcp_control *tcb;

	tcb = look_for_control(port);
	if(tcb == NULL) {
		printf("port not used\n");
		return -1;
	}

	data_size = RECEIVE_BUFFER_SIZE - tcb->local_window;
	while(data_size == 0)
		data_size = RECEIVE_BUFFER_SIZE - tcb->local_window;

	read_data_size = data_size > size ? size : data_size;
	if((tcb->rx_buffer_r_ptr + read_data_size) > (tcb->rx_buffer + RECEIVE_BUFFER_SIZE)) {
		uint32_t exceed_size = (tcb->rx_buffer_r_ptr + read_data_size) -
			(tcb->rx_buffer + RECEIVE_BUFFER_SIZE);

		memcpy(buffer, tcb->rx_buffer_r_ptr, read_data_size - exceed_size);
		memcpy(buffer + (read_data_size - exceed_size), tcb->rx_buffer,
				exceed_size);
		tcb->rx_buffer_r_ptr = tcb->rx_buffer + exceed_size;
	} else {
		memcpy(buffer, tcb->rx_buffer_r_ptr, read_data_size);
		tcb->rx_buffer_r_ptr += read_data_size;
	}

	tcb->local_window = tcb->local_window + read_data_size;

	return read_data_size;
}

int tcpv4_distribute_data(uint16_t port, uint8_t *buffer, uint32_t size)
{
}

void tcpv4_init()
{
	tcp_control_list = NULL;
	write_buffer = (uint8_t *)malloc(MAX_SEGMENT_SIZE);
}

void tcpv4_exit()
{
	free(write_buffer);
}
