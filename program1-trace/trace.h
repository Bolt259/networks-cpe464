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

// ethernet header structure
struct ethernet_hdr {
    unsigned char dest[6]; // destination MAC address
    unsigned char src[6]; // source MAC address
    unsigned short ether_type;    // ethernet type
};

// ip header structure
struct ip_hdr {
    unsigned char ver_ihl; // version and header length
    unsigned char tos; // type of service
    unsigned short total_len; // total length
    unsigned short id; // identification
    unsigned short frag_off; // fragment offset
    unsigned char ttl; // time to live
    unsigned char proto; // protocol
    unsigned short check; // checksum
    unsigned int src_ip; // source IP address
    unsigned int dest_ip; // destination IP address
};

// function declarations
void ethernet(const unsigned char *packet);
void icmp(const unsigned char *packet);
void arp(const unsigned char *packet);
void ip(const unsigned char *packet);
void tcp(const unsigned char *packet);
void udp(const unsigned char *packet);

#endif // TRACE_H