#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pcap.h>

#define ETHER_TYPE_IP 0x0800
#define ETHER_TYPE_ARP 0x0806

#define ETHER_HDR_LEN 14

// // ethernet header structure
// struct ethernet_hdr {
//     u_int8_t dest[6]; // destination MAC address
//     u_int8_t src[6]; // source MAC address
//     unsigned short ether_type;    // ethernet type
// };

// // ip header structure
// struct ip_hdr {
//     u_int8_t ver_ihl; // version and header length
//     u_int8_t tos; // type of service
//     unsigned short total_len; // total length
//     unsigned short id; // identification
//     unsigned short frag_off; // fragment offset
//     u_int8_t ttl; // time to live
//     u_int8_t proto; // protocol
//     unsigned short check; // checksum
//     unsigned int src_ip; // source IP address
//     unsigned int dest_ip; // destination IP address
// };

// pseudo header struct for TCP checksum calculation
struct pseudo_hdr {
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