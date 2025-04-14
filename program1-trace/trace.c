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
void udp();

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
    const u_int8_t *packet;

    int pkt_cnt = 1;
    
    while ((result = pcap_next_ex(handle, &header, &packet)) >= 0) {
        if (pkt_cnt == 1) {
            printf("Packet number: %d  Packet Len: %d\n", pkt_cnt++, header->len);
        } else {
            printf("\nPacket number: %d  Packet Len: %d\n", pkt_cnt++, header->len);
        }

        // process each packet

        ethernet(packet);   // pull ethernet header

        // struct ethernet_hdr *eth = (struct ethernet_hdr *)packet; // reintroducing ethernet header struct
        // u_short eth_type = ntohs(*(u_int16_t *)(packet + 12)); // defining eth_type
        
        // determine the type of packet and pull the appropriate header
        u_int16_t eth_type = ntohs(*(u_int16_t *)(packet + 12));  // dereference pointer
        switch (eth_type) {
            case ETHER_TYPE_IP: {
                int ip_hdr_len;
                ip_hdr_len = (packet[14] & 0x0F) * 4; // calculate header length
                ip(packet, ip_hdr_len);   // pull IP header
        
                // determine ICMP, TCP, or UDP
                int protocol = *(u_int8_t *)(packet + 23); // no need to worry about endianness with single-byte operations
        
                // pull header length
                int header_len = ETHER_HDR_LEN + ip_hdr_len; // calculate header length
        
                switch (protocol) {
                    case 1:
                        icmp(packet + header_len);   // pull ICMP header
                        break;
                    case 6:
                        tcp(packet + header_len);   // pull TCP header
                        break;
                    case 17:
                        udp(packet + header_len);   // pull UDP header
                        break;
                    default:
                        printf("Unknown protocol\n");
                        break;
                }
                break;
            }
            case ETHER_TYPE_ARP:
                arp(packet);   // pull ARP header
                break;
            default:
                printf("Unknown packet type\n");
                break;
        }
        // if (ntohs(*(u_int16_t *)(packet + 12)) == ETHER_TYPE_IP) {
        //     ip(packet);   // pull ip header

        //     // determine ICMP TCP or UDP
        //     int protocol = *(u_int8_t *)(packet + 23); // no need to worry abt endianess with single byte operations - dereference pointer

        //     if (protocol == 1) {
        //         icmp(packet);   // pull icmp header
        //     } else if (protocol == 6) {
        //         tcp(packet);   // pull tcp header
        //     } else if (protocol == 17) {
        //         udp(packet);   // pull udp header
        //     } else {
        //         printf("Unknown IP protocol\n");
        //     }
        
        // } else if (ntohs(*(u_int16_t *)(packet + 12)) == ETHER_TYPE_ARP) {
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
void ethernet(const u_int8_t *packet) {

    u_int8_t eth_dest[6]; // destination MAC address
    u_int8_t eth_src[6]; // source MAC address
    // u_int16_t eth_type = ntohs(*(u_int16_t *)(packet + 12));
    u_int16_t eth_type;

    memcpy(eth_dest, packet, 6);
    memcpy(eth_src, packet + 6, 6);
    memcpy(&eth_type, packet + 12, 2);
    eth_type = ntohs(eth_type);

    printf("\n\tEthernet Header\n");
    printf(
        "\t\tDest MAC: %x:%x:%x:%x:%x:%x\n",
        eth_dest[0], eth_dest[1], eth_dest[2], eth_dest[3], eth_dest[4], eth_dest[5]
    );
    printf(
        "\t\tSource MAC: %x:%x:%x:%x:%x:%x\n",
        eth_src[0], eth_src[1], eth_src[2], eth_src[3], eth_src[4], eth_src[5]
    );
    printf(
        "\t\tType: %s\n",
        (eth_type == ETHER_TYPE_IP) ? "IP" :
        (eth_type == ETHER_TYPE_ARP) ? "ARP" :
        "Unknown"
    );
}

// ip header
void ip(const u_int8_t *packet, int ip_hdr_len) {
    // check if valid IP version
    // if ((*(u_int8_t *)(packet + 14) & 0xF0) != 0x40) {
    //     printf("Invalid IP version\n");
    // }
    
    // checksum calcs
    u_int16_t expected = ntohs(*(u_int16_t *)(packet + 24));
    u_int16_t actual = in_cksum((u_int16_t *)packet, ip_hdr_len);
    
    printf("\n\tIP Header\n");
    printf("\t\tIP PDU Len: %d\n", ntohs(*(u_int16_t *)(packet + 16)));
    printf("\t\tHeader Len (bytes): %d\n", ip_hdr_len);
    printf("\t\tTTL: %d\n", packet[22]);
    printf(
        "\t\tProtocol: %s\n", 
        (packet[23] == 1) ? "ICMP" : 
        (packet[23] == 6) ? "TCP" : 
        (packet[23] == 17) ? "UDP" : "Unknown"
    );
    printf(
        "\t\tChecksum: %s (0x%04x)\n", 
        (actual == 0) ? "Correct" : "Incorrect",    // actual checksum is 0 if valid
        expected
    );
    printf(
        "\t\tSender IP: %d.%d.%d.%d\n",
        packet[26], packet[27], packet[28], packet[29]
    );
    printf(
        "\t\tDest IP: %d.%d.%d.%d\n",
        packet[30], packet[31], packet[32], packet[33]
    );
}

// icmp header - check type cause idk
void icmp(const u_int8_t *packet) {
    printf("\n\tICMP Header\n");
    if (packet[0] == 0) {
        printf("\t\tType: Reply\n");
    } else if (packet[0] == 8) {
        printf("\t\tType: Request\n");
    } else {
        printf("\t\tType: %d\n", packet[0]);
    }
}

// arp header
void arp(const u_int8_t *packet) {
    printf("\n\tARP header\n");
    printf(
        "\t\tOpcode: %s\n", 
        (ntohs(*(u_int16_t *)(packet + 20)) == 1) ? "Request" : 
        (ntohs(*(u_int16_t *)(packet + 20)) == 2) ? "Reply" : "Unknown"
    );
    printf(
        "\t\tSender MAC: %x:%x:%x:%x:%x:%x\n",
        packet[22], packet[23], packet[24], packet[25], packet[26], packet[27]
    );
    printf(
        "\t\tSender IP: %d.%d.%d.%d\n",
        packet[28], packet[29], packet[30], packet[31]
    );
    printf(
        "\t\tTarget MAC: %x:%x:%x:%x:%x:%x\n",
        packet[32], packet[33], packet[34], packet[35], packet[36], packet[37]
    );
    printf(
        "\t\tTarget IP: %d.%d.%d.%d\n",
        packet[38], packet[39], packet[40], packet[41]
    );
}

// tcp header
void tcp(const u_int8_t *packet) {
    printf("\n\tTCP Header\n");
    printf("Source Port: %d\n", ntohs(*(u_int16_t *)(packet + 34)));
    printf("Dest Port: %d\n", ntohs(*(u_int16_t *)(packet + 36)));
    printf("Sequence Number: %u\n", ntohl(*(u_int32_t *)(packet + 38)));
    printf("Acknowledgment Number: %u\n", ntohl(*(u_int32_t *)(packet + 42)));
    printf("Data Offset: %d\n", (packet[46] >> 4) & 0x0F);
    printf("Flags: 0x%02x\n", packet[47]);
    printf("Window Size: %d\n", ntohs(*(u_int16_t *)(packet + 48)));
    printf("Checksum: 0x%04x\n", ntohs(*(u_int16_t *)(packet + 50)));
    printf("Urgent Pointer: %d\n", ntohs(*(u_int16_t *)(packet + 52)));
}
// udp header
void udp(const u_int8_t *packet) {
    printf("\n\tUDP Header\n");
    printf("Source Port: %d\n", ntohs(*(u_int16_t *)(packet + 34)));
    printf("Dest Port: %d\n", ntohs(*(u_int16_t *)(packet + 36)));
    printf("Length: %d\n", ntohs(*(u_int16_t *)(packet + 38)));
    printf("Checksum: 0x%04x\n", ntohs(*(u_int16_t *)(packet + 40)));
}

