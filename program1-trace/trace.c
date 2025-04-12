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

    pcap_close(handle);
    return EXIT_SUCCESS;
}

