// main.c
// Lukas Shipley

#include "trace.h"
#include "checksum.h"

// function declarations
void ethernet();
void icmp();
void arp();
void ip();
void tcp();

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pcap_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(argv[1], errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Error opening pcap file: %s\n", errbuf);
        return EXIT_FAILURE;
    }

    printf("Successfully opened pcap file: %s\n", argv[1]);

    int result;
    struct pcap_pkthdr *header;
    const unsigned char *packet;

    int pkt_cnt = 1;
    
    while ((result = pcap_next_ex(handle, &header, &packet)) >= 0) {
        printf("Packet number: %d  Packet length: %d\n\n", pkt_cnt++, header->len);

        // process each packet

        ethernet(packet);   // pull ethernet header

        // struct ethernet_hdr *eth = (struct ethernet_hdr *)packet; // reintroducing ethernet header struct
        // u_short eth_type = ntohs(*(unsigned short *)(packet + 12)); // defining eth_type
        
        // determine the type of packet and pull the appropriate header
        unsigned short eth_type = ntohs(*(unsigned short *)(packet + 12));  // dereference pointer
        switch (eth_type) {
            case ETHER_TYPE_IP:
                ip(packet);   // pull ip header

                struct ip_hdr *ip = (struct ip_hdr *)(packet + ETHER_HDR_LEN);
                memcpy(ip, packet + ETHER_HDR_LEN, sizeof(struct ip_hdr));
                int ihl_bytes = (ip->ver_ihl & 0x0F) * 4; // get header length in bytes

                printf("IP Header Info from Struct:\n");
                printf("Version: %d\n", (ip->ver_ihl >> 4) & 0x0F);
                printf("Header Length: %d bytes\n", (ip->ver_ihl & 0x0F) * 4);
                printf("Type of Service: %d\n", ip->tos);
                printf("Total Length: %d bytes\n", ntohs(ip->total_len));
                printf("Identification: %d\n", ntohs(ip->id));
                printf("Flags: %d\n", (ntohs(ip->frag_off) >> 13) & 0x07);
                printf("Fragment Offset: %d\n", ntohs(ip->frag_off) & 0x1FFF);
                printf("Time to Live: %d\n", ip->ttl);
                printf("Protocol: %d\n", ip->proto);
                printf("Header Checksum: 0x%04x\n", ntohs(ip->check));
                printf("Source IP: %d.%d.%d.%d\n",
                    (ip->src_ip >> 24) & 0xFF, (ip->src_ip >> 16) & 0xFF, (ip->src_ip >> 8) & 0xFF, ip->src_ip & 0xFF);
                printf("Destination IP: %d.%d.%d.%d\n",
                    (ip->dest_ip >> 24) & 0xFF, (ip->dest_ip >> 16) & 0xFF, (ip->dest_ip >> 8) & 0xFF, ip->dest_ip & 0xFF);

                // determine ICMP TCP or UDP
                int protocol = *(unsigned char *)(packet + 23); // no need to worry abt endianess with single byte operations - dereference pointer
                switch (protocol) {
                    case 1:
                        icmp(packet);   // pull icmp header
                        break;
                    case 6:
                        tcp(packet);   // pull tcp header
                        break;
                    case 17:
                        udp(packet);   // pull udp header
                        break;
                    default:
                        printf("Unknown IP protocol\n");
                }

                break;
            case ETHER_TYPE_ARP:
                arp(packet);   // pull arp header
                break;
            default:
                printf("Unknown packet type\n");
        }

        // if (ntohs(*(unsigned short *)(packet + 12)) == ETHER_TYPE_IP) {
        //     ip(packet);   // pull ip header

        //     // determine ICMP TCP or UDP
        //     int protocol = *(unsigned char *)(packet + 23); // no need to worry abt endianess with single byte operations - dereference pointer

        //     if (protocol == 1) {
        //         icmp(packet);   // pull icmp header
        //     } else if (protocol == 6) {
        //         tcp(packet);   // pull tcp header
        //     } else if (protocol == 17) {
        //         udp(packet);   // pull udp header
        //     } else {
        //         printf("Unknown IP protocol\n");
        //     }
        
        // } else if (ntohs(*(unsigned short *)(packet + 12)) == ETHER_TYPE_ARP) {
        //     arp(packet);   // pull arp header
        // } else {
        //     printf("Unknown packet type\n");
        // }

    }
    if (result == -1) {
        fprintf(stderr, "Error reading pcap file: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        return EXIT_FAILURE;
    }

    pcap_close(handle);
    return EXIT_SUCCESS;
}


// ethernet header
void ethernet(const unsigned char *packet) {

    struct ethernet_hdr *eth = (struct ethernet_hdr *)packet; // reintroducing ethernet header struct
    unsigned short eth_type = ntohs(*(unsigned short *)(packet + 12)); // defining eth_type

    printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        eth->dest[0], eth->dest[1], eth->dest[2], eth->dest[3], eth->dest[4], eth->dest[5]);
    printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            eth->src[0], eth->src[1], eth->src[2], eth->src[3], eth->src[4], eth->src[5]);
    printf("Ethernet Type: 0x%04x\n", eth_type);
}

// ip header
void ip(const unsigned char *packet) {
    printf("IP Header:\n");
    printf("Version: %d\n", (packet[14] >> 4) & 0x0F);
    printf("Header Length: %d bytes\n", (packet[14] & 0x0F) * 4);
    printf("Type of Service: %d\n", packet[15]);
    printf("Total Length: %d bytes\n", ntohs(*(unsigned short *)(packet + 16)));
    printf("Identification: %d\n", ntohs(*(unsigned short *)(packet + 18)));
    printf("Flags: %d\n", (ntohs(*(unsigned short *)(packet + 20)) >> 13) & 0x07);
    printf("Fragment Offset: %d\n", ntohs(*(unsigned short *)(packet + 20)) & 0x1FFF);
    printf("Time to Live: %d\n", packet[22]);
    printf("Protocol: %d\n", packet[23]);
    printf("Header Checksum: 0x%04x\n", ntohs(*(unsigned short *)(packet + 24)));
    printf("Source IP: %d.%d.%d.%d\n",
        packet[26], packet[27], packet[28], packet[29]);
    printf("Destination IP: %d.%d.%d.%d\n",
        packet[30], packet[31], packet[32], packet[33]);
}

// icmp header
void icmp(const unsigned char *packet) {
    printf("ICMP Header:\n");
    printf("Type: %d\n", packet[34]);
    printf("Code: %d\n", packet[35]);
    printf("Checksum: 0x%04x\n", ntohs(*(unsigned short *)(packet + 36)));
    printf("Identifier: %d\n", ntohs(*(unsigned short *)(packet + 38)));
    printf("Sequence Number: %d\n", ntohs(*(unsigned short *)(packet + 40)));
}
// arp header
void arp(const unsigned char *packet) {
    printf("ARP Header:\n");
    printf("Hardware Type: %d\n", ntohs(*(unsigned short *)(packet + 14)));
    printf("Protocol Type: 0x%04x\n", ntohs(*(unsigned short *)(packet + 16)));
    printf("Hardware Address Length: %d\n", packet[18]);
    printf("Protocol Address Length: %d\n", packet[19]);
    printf("Operation: %d\n", ntohs(*(unsigned short *)(packet + 20)));
    printf("Sender Hardware Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        packet[22], packet[23], packet[24], packet[25], packet[26], packet[27]);
    printf("Sender Protocol Address: %d.%d.%d.%d\n",
        packet[28], packet[29], packet[30], packet[31]);
    printf("Target Hardware Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        packet[32], packet[33], packet[34], packet[35], packet[36], packet[37]);
    printf("Target Protocol Address: %d.%d.%d.%d\n",
        packet[38], packet[39], packet[40], packet[41]);
}
// tcp header
void tcp(const unsigned char *packet) {
    printf("TCP Header:\n");
    printf("Source Port: %d\n", ntohs(*(unsigned short *)(packet + 34)));
    printf("Dest Port: %d\n", ntohs(*(unsigned short *)(packet + 36)));
    printf("Sequence Number: %u\n", ntohl(*(unsigned int *)(packet + 38)));
    printf("Acknowledgment Number: %u\n", ntohl(*(unsigned int *)(packet + 42)));
    printf("Data Offset: %d\n", (packet[46] >> 4) & 0x0F);
    printf("Flags: 0x%02x\n", packet[47]);
    printf("Window Size: %d\n", ntohs(*(unsigned short *)(packet + 48)));
    printf("Checksum: 0x%04x\n", ntohs(*(unsigned short *)(packet + 50)));
    printf("Urgent Pointer: %d\n", ntohs(*(unsigned short *)(packet + 52)));
}
// udp header
void udp(const unsigned char *packet) {
    printf("UDP Header:\n");
    printf("Source Port: %d\n", ntohs(*(unsigned short *)(packet + 34)));
    printf("Dest Port: %d\n", ntohs(*(unsigned short *)(packet + 36)));
    printf("Length: %d\n", ntohs(*(unsigned short *)(packet + 38)));
    printf("Checksum: 0x%04x\n", ntohs(*(unsigned short *)(packet + 40)));
}

