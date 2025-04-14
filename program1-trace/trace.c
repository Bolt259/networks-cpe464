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

    printf("\n");

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

                // determine ICMP TCP or UDP
                int protocol = *(unsigned char *)(packet + 23); // no need to worry abt endianess with single byte operations - dereference pointer
                
                // pull header len
                int header_len = ETHER_HDR_LEN + (packet[14] & 0x0F) * 4; // calculate header length

                switch (protocol) {
                    case 1:
                        icmp(packet + header_len);   // pull icmp header
                        break;
                    case 6:
                        tcp(packet + header_len);   // pull tcp header
                        break;
                    case 17:
                        udp(packet + header_len);   // pull udp header
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

    unsigned char eth_dest[6]; // destination MAC address
    unsigned char eth_src[6]; // source MAC address
    // unsigned short eth_type = ntohs(*(unsigned short *)(packet + 12));
    unsigned short eth_type;

    memcpy(eth_dest, packet, 6);
    memcpy(eth_src, packet + 6, 6);
    memcpy(&eth_type, packet + 12, 2);
    eth_type = ntohs(eth_type);

    printf("\tEthernet Header:\n");
    printf("\t\tDest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        eth_dest[0], eth_dest[1], eth_dest[2], eth_dest[3], eth_dest[4], eth_dest[5]);
    printf("\t\tSource MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            eth_src[0], eth_src[1], eth_src[2], eth_src[3], eth_src[4], eth_src[5]);
    printf("\t\tType: %s\n",
        (eth_type == ETHER_TYPE_IP) ? "IP" :
        (eth_type == ETHER_TYPE_ARP) ? "ARP" :
        "Unknown"
    );
}

// ip header
void ip(const unsigned char *packet) {
    printf("\tIP Header\n");
    printf("\t\tIP PDU Len: %d\n", ntohs(*(unsigned short *)(packet + 16)));
    printf("\t\tHeader Len (bytes): %d\n", (packet[14] & 0x0F) * 4);
    printf("\t\tTTL: %d\n", packet[22]);
    printf("\t\tProtocol: %s\n", 
        (packet[23] == 1) ? "ICMP" : 
        (packet[23] == 6) ? "TCP" : 
        (packet[23] == 17) ? "UDP" : "Unknown"
    );

    unsigned short checksum = ntohs(*(unsigned short *)(packet + 24));
    
    printf("\t\tChecksum: %s (0x%04x)\n", 
        (checksum == 0) ? "Correct" : "Incorrect",
        checksum
    );
    printf("\t\tSender IP: %d.%d.%d.%d\n",
        packet[26], packet[27], packet[28], packet[29]);
    printf("\t\tDest IP: %d.%d.%d.%d\n",
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

