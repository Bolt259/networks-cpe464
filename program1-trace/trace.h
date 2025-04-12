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

// function declarations
void ethernet();
void icmp();
void arp();
void ip();
void tcp();
void udp();

#endif // TRACE_H