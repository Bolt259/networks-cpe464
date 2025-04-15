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
// int is_tcp_chksum_valid(const u_int8_t *packet, int ip_hdr_len);
char *port_name(u_int16_t port);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <pcap_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_offline(argv[1], errbuf);
    if (handle == NULL)
    {
        fprintf(stderr, "Error opening pcap file: %s\n", errbuf);
        return EXIT_FAILURE;
    }

    printf("\n");

    int result;
    struct pcap_pkthdr *header;
    const u_int8_t *packet;

    int pkt_cnt = 1;

    while ((result = pcap_next_ex(handle, &header, &packet)) >= 0)
    {
        if (pkt_cnt == 1)
        {
            printf("Packet number: %d  Packet Len: %d\n", pkt_cnt++, header->len);
        }
        else
        {
            printf("\nPacket number: %d  Packet Len: %d\n", pkt_cnt++, header->len);
        }

        // process each packet

        // ETHERNET LAYER
        ethernet(packet); // pull ethernet header

        // determine the type of packet and pull the appropriate header
        u_int16_t eth_type = ntohs(*(u_int16_t *)(packet + 12)); // dereference pointer
        switch (eth_type)
        {
        case ETHER_TYPE_IP:
        {
            int ip_hdr_len;
            ip_hdr_len = (packet[14] & 0x0F) * 4; // calculate header length

            // IP LAYER
            ip(packet, ip_hdr_len); // pull IP header

            // determine ICMP, TCP, or UDP
            int protocol = *(u_int8_t *)(packet + 23); // no need to worry about endianness with single-byte operations

            // pull header length
            u_int8_t header_len = ETHER_HDR_LEN + ip_hdr_len; // calculate header length

            u_int8_t *prev_hdr_block_len = (u_int8_t *)(packet + header_len);

            switch (protocol)
            {
            case 1:
                icmp(prev_hdr_block_len); // pull ICMP header
                break;
            case 6:
                tcp(packet, ip_hdr_len); // pull TCP header
                break;
            case 17:
                udp(prev_hdr_block_len); // pull UDP header
                break;
            default:
                break;
            }
            break;
        }
        case ETHER_TYPE_ARP:
            arp(packet); // pull ARP header
            break;
        default:
            printf("Unknown packet type\n");
            break;
        }
    }
    if (result == -1)
    {
        fprintf(stderr, "Error reading pcap file: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        return EXIT_FAILURE;
    }

    pcap_close(handle);
    return EXIT_SUCCESS;
}

// ethernet header
void ethernet(const u_int8_t *packet)
{

    u_int8_t eth_dest[6]; // destination MAC address
    u_int8_t eth_src[6];  // source MAC address
    // u_int16_t eth_type = ntohs(*(u_int16_t *)(packet + 12));
    u_int16_t eth_type;

    memcpy(eth_dest, packet, 6);
    memcpy(eth_src, packet + 6, 6);
    memcpy(&eth_type, packet + 12, 2);
    eth_type = ntohs(eth_type);

    printf("\n\tEthernet Header\n");
    printf(
        "\t\tDest MAC: %x:%x:%x:%x:%x:%x\n",
        eth_dest[0], eth_dest[1], eth_dest[2], eth_dest[3], eth_dest[4], eth_dest[5]);
    printf(
        "\t\tSource MAC: %x:%x:%x:%x:%x:%x\n",
        eth_src[0], eth_src[1], eth_src[2], eth_src[3], eth_src[4], eth_src[5]);
    printf(
        "\t\tType: %s\n",
        (eth_type == ETHER_TYPE_IP) ? "IP" : (eth_type == ETHER_TYPE_ARP) ? "ARP"
                                                                          : "Unknown");
}

// ip header
void ip(const u_int8_t *packet, int ip_hdr_len)
{
    // checksum calcs
    u_int16_t expected = ntohs(*(u_int16_t *)(packet + 24));
    u_int16_t actual = in_cksum((u_int16_t *)(packet + ETHER_HDR_LEN), ip_hdr_len);

    // printf("\t\tPacket pointer: %p\n", (void *)packet);
    // printf("\t\tPacket+ETHER_HDR_LEN pointer: %p\n", (void *)(packet + ETHER_HDR_LEN));
    // printf("\t\tData at Packet+ETHER_HDR_LEN: 0x%02x\n", *(packet + ETHER_HDR_LEN));

    printf("\n\tIP Header\n");
    printf("\t\tIP PDU Len: %d\n", ntohs(*(u_int16_t *)(packet + 16)));
    printf("\t\tHeader Len (bytes): %d\n", ip_hdr_len);
    printf("\t\tTTL: %d\n", packet[22]);
    printf(
        "\t\tProtocol: %s\n",
        (packet[23] == 1) ? "ICMP" : (packet[23] == 6) ? "TCP"
                                 : (packet[23] == 17)  ? "UDP"
                                                       : "Unknown");
    printf(
        "\t\tChecksum: %s (0x%04x)\n",
        (actual == 0) ? "Correct" : "Incorrect", // actual checksum is 0 if valid
        expected);
    
    // printf("\t\tactual: 0x%04x\n", actual);     // REMOVE THIS FOR DEBUGGING!!!

    printf(
        "\t\tSender IP: %d.%d.%d.%d\n",
        packet[26], packet[27], packet[28], packet[29]);
    printf(
        "\t\tDest IP: %d.%d.%d.%d\n",
        packet[30], packet[31], packet[32], packet[33]);
}

// icmp header - check type cause idk
void icmp(const u_int8_t *packet)
{
    printf("\n\tICMP Header\n");
    if (packet[0] == 0)
    {
        printf("\t\tType: Reply\n");
    }
    else if (packet[0] == 8)
    {
        printf("\t\tType: Request\n");
    }
    else
    {
        printf("\t\tType: %d\n", packet[0]);
    }
}

// arp header
void arp(const u_int8_t *packet)
{
    printf("\n\tARP header\n");
    printf(
        "\t\tOpcode: %s\n",
        (ntohs(*(u_int16_t *)(packet + 20)) == 1) ? "Request" : (ntohs(*(u_int16_t *)(packet + 20)) == 2) ? "Reply"
                                                                                                          : "Unknown");
    printf(
        "\t\tSender MAC: %x:%x:%x:%x:%x:%x\n",
        packet[22], packet[23], packet[24], packet[25], packet[26], packet[27]);
    printf(
        "\t\tSender IP: %d.%d.%d.%d\n",
        packet[28], packet[29], packet[30], packet[31]);
    printf(
        "\t\tTarget MAC: %x:%x:%x:%x:%x:%x\n",
        packet[32], packet[33], packet[34], packet[35], packet[36], packet[37]);
    printf(
        "\t\tTarget IP: %d.%d.%d.%d\n",
        packet[38], packet[39], packet[40], packet[41]);
}

// tcp header
void tcp(const u_int8_t *packet, int ip_hdr_len)
{
    u_int16_t ip_total_len = ntohs(*(u_int16_t *)(packet + 16));
    u_int32_t tcp_len = ip_total_len - ip_hdr_len;
    const u_int8_t *tcp_start = packet + ETHER_HDR_LEN + ip_hdr_len; // start of TCP segment in memory

    // build pseudo header
    struct pseudo_hdr
    {
        uint32_t src_ip_addr;
        uint32_t dest_ip_addr;
        uint8_t zero;
        uint8_t protocol;
        uint16_t tcp_len;
    };

    struct pseudo_hdr pseudo;
    pseudo.src_ip_addr = ntohl(*(u_int32_t *)(packet + 26));  // 4 bytes of source IP starting at 26
    pseudo.dest_ip_addr = ntohl(*(u_int32_t *)(packet + 30)); // 4 bytes of destination IP starting at 30
    pseudo.zero = 0;
    pseudo.protocol = packet[23];
    pseudo.tcp_len = *(u_int16_t *)(packet + 16) - ip_hdr_len; // 2 bytes of TCP length starting at 16 kept in network byte order for checksum calc

    // allocate buff on stack: pseudo_hdr (12 bytes) + TCP segment (variable length)
    uint8_t buff[sizeof(struct pseudo_hdr) + tcp_len];
    memcpy(buff, &pseudo, sizeof(struct pseudo_hdr));                       // copy pseudo header to buff
    memcpy(buff + sizeof(struct pseudo_hdr), packet + ip_hdr_len, tcp_len); // copy TCP segment to buff

    // checksum calcs
    u_int16_t expected = ntohs(*(u_int16_t *)(tcp_start + 16));                          // pull checksum from TCP header
    u_int16_t actual = in_cksum((u_int16_t *)buff, sizeof(struct pseudo_hdr) + tcp_len); // calculate checksum

    // int valid = is_tcp_chksum_valid(packet, ip_hdr_len);

    printf("\n\tTCP Header\n");
    printf("\t\tSegment Length: %d\n", tcp_len);

    // char *src_port = port_name(ntohs(*(u_int16_t *)(tcp_start)));
    // char *dest_port = port_name(ntohs(*(u_int16_t *)(tcp_start + 2)));
    // printf("\t\tSource Port: %s\n", src_port);
    // printf("\t\tDest Port: %s\n", dest_port);
    printf("\t\tSource Port:  %s\n", port_name(ntohs(*(u_int16_t *)(tcp_start))));
    printf("\t\tDest Port:  %s\n", port_name(ntohs(*(u_int16_t *)(tcp_start + 2))));

    printf("\t\tSequence Number: %u\n", ntohl(*(u_int32_t *)(tcp_start + 4)));
    printf("\t\tACK Number: %u\n", ntohl(*(u_int32_t *)(tcp_start + 8)));
    printf("\t\tData Offset (bytes): %d\n", (tcp_start[12] / 16) * 4); // mask off lower nibble

    printf("\t\tSYN Flag: %s\n", (tcp_start[13] & 0x02) ? "Yes" : "No");
    printf("\t\tRST Flag: %s\n", (tcp_start[13] & 0x04) ? "Yes" : "No");
    printf("\t\tFIN Flag: %s\n", (tcp_start[13] & 0x01) ? "Yes" : "No");
    printf("\t\tACK Flag: %s\n", (tcp_start[13] & 0x10) ? "Yes" : "No");

    printf("\t\tWindow Size: %d\n", ntohs(*(u_int16_t *)(tcp_start + 14)));
    printf(
        "\t\tChecksum: %s (0x%04x)\n",
        (actual == 0) ? "Correct" : "Incorrect",
        expected
    );
}

// udp header
void udp(const u_int8_t *packet)
{
    printf("\n\tUDP Header\n");
    printf("\t\tSource Port: %d\n", ntohs(*(u_int16_t *)(packet)));
    printf("\t\tDest Port: %d\n", ntohs(*(u_int16_t *)(packet + 2)));
}

// // tcp checksum validation
// int is_tcp_chksum_valid(const u_int8_t *packet, int ip_hdr_len)
// {
//     u_int16_t ip_total_len = ntohs(*(u_int16_t *)(packet + 16));
//     u_int32_t tcp_len = ip_total_len - ip_hdr_len;
//     const u_int8_t *tcp_start = packet + ETHER_HDR_LEN + ip_hdr_len; // start of TCP segment in memory

//     // build pseudo header
//     struct pseudo_hdr
//     {
//         uint32_t src_ip_addr;
//         uint32_t dest_ip_addr;
//         uint8_t zero;
//         uint8_t protocol;
//         uint16_t tcp_len;
//     };

//     struct pseudo_hdr pseudo;
//     pseudo.src_ip_addr = ntohl(*(u_int32_t *)(packet + 26));  // 4 bytes of source IP starting at 26
//     pseudo.dest_ip_addr = ntohl(*(u_int32_t *)(packet + 30)); // 4 bytes of destination IP starting at 30
//     pseudo.zero = 0;
//     pseudo.protocol = packet[23];
//     pseudo.tcp_len = *(u_int16_t *)(packet + 16) - ip_hdr_len; // 2 bytes of TCP length starting at 16 kept in network byte order for checksum calc

//     // allocate buff on stack: pseudo_hdr (12 bytes) + TCP segment (variable length)
//     uint8_t buff[sizeof(struct pseudo_hdr) + tcp_len];
//     memcpy(buff, &pseudo, sizeof(struct pseudo_hdr));                       // copy pseudo header to buff
//     memcpy(buff + sizeof(struct pseudo_hdr), packet + ip_hdr_len, tcp_len); // copy TCP segment to buff

//     // checksum calcs
//     u_int16_t expected = ntohs(*(u_int16_t *)(tcp_start + 16));                          // pull checksum from TCP header
//     u_int16_t actual = in_cksum((u_int16_t *)buff, sizeof(struct pseudo_hdr) + tcp_len); // calculate checksum

//     return actual == 0; // check if checksum is valid
// }

// helper function to get port name
char *port_name(u_int16_t port)
{
    switch (port)
    {
    case 80:
        return "HTTP";
    case 21:
        return "FTP";
    case 23:
        return "Telnet";
    case 25:
        return "SMTP";
    case 110:
        return "POP3";
    default:
    {
        static char buff[16];
        snprintf(buff, sizeof(buff), "%d", port);
        return buff;
    }
    }
}
