#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pcap.h>
#include <arpa/inet.h>

#define ETHER_TYPE_IP 0x0800
#define ETHER_TYPE_ARP 0x0806

#define ETHER_HDR_LEN 14
#define IP_SRC_OFFSET 26
#define IP_DEST_OFFSET 30
#define TCP_CHECKSUM_OFFSET 16

#define PROTOCOL_ICMP 1
#define PROTOCOL_TCP 6
#define PROTOCOL_UDP 17

#define PROTOCOL_HTTP 80
#define PROTOCOL_FTP 21
#define PROTOCOL_TELNET 23
#define PROTOCOL_SMTP 25
#define PROTOCOL_DNS 53
#define PROTOCOL_POP3 110

// pseudo header struct for TCP checksum calculation
struct pseudo_hdr
{
    u_int32_t src_ip_addr;
    u_int32_t dest_ip_addr;
    u_int8_t zero;
    u_int8_t protocol;
    u_int16_t tcp_len;
};

// function declarations
void ethernet(const u_int8_t *packet);
void icmp(const u_int8_t *packet);
void arp(const u_int8_t *packet);
void ip(const u_int8_t *packet, int ip_hdr_len);
void tcp(const u_int8_t *packet, int ip_hdr_len);
void udp(const u_int8_t *packet);

// helper function to get port name
char *port_name(u_int16_t port);

#endif // TRACE_H