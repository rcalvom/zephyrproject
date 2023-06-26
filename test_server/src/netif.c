#include "netif.h"

pcap_t *interface;


pcap_t* set_interface_modes(pcap_t* interface_handler){
    int ret;
    log_debug("Setting device modes of operation on network interface");
    ret = pcap_set_promisc(interface_handler, 1);
    if((ret != 0) && (ret != PCAP_ERROR_ACTIVATED)){
        log_error("couldn't not activate promisuous mode\n");
        return NULL;
    }
    ret = pcap_set_snaplen(interface_handler, 1222);
    if((ret != 0) && (ret != PCAP_ERROR_ACTIVATED)) {
        log_error("couldn't not set snaplen\n");
        return NULL;
    }
    ret = pcap_set_timeout(interface_handler, 200);
    if((ret != 0) && (ret != PCAP_ERROR_ACTIVATED)) {
        log_error("couldn't not set timeout\n");
        return NULL;
    }
    ret = pcap_set_buffer_size(interface_handler, 1222 * 1100);
    if((ret != 0) && (ret != PCAP_ERROR_ACTIVATED)) {
        log_error("couldn't not set buffer size\n" );
        return NULL;
    }
    return interface_handler;
}


pcap_t* open_interface(char* interface_name) {
    char error_buffer[256];
    pcap_t *interface_handler = pcap_create(interface_name, error_buffer);
    if(interface_handler == NULL) {
        log_error("The interface %s is not supported by pcap and cannot be opened: %s\n", interface_name, error_buffer);
        return NULL;
    }
    interface_handler = set_interface_modes(interface_handler);
    if(interface_handler == NULL) {
        log_error("Error configuring Interface");
        return NULL;
    }
    if(pcap_activate(interface_handler) != 0) {
        log_error("pcap activate error %s\n", pcap_geterr(interface_handler));
        return NULL;
    }
    return interface_handler;
}

void print_hex(unsigned const char * const bin_data, size_t len){
    size_t i;
    for(i = 0; i < len; i++){
        printf("%.2X ", bin_data[i]);
    }
    printf("\n");
}

void pcap_callback(unsigned char * user, const struct pcap_pkthdr *pkt_header, const unsigned char *pkt_data) {
    log_info("Receiving <=  network callback user: %s len: %d caplen: %d\n", user, pkt_header->len, pkt_header->caplen);
    print_hex(pkt_data, pkt_header->len);
    
    process_packet_packetdrill((void*)pkt_data, pkt_header->len);

}

int print_output(void *p, int len){
    if(len > 0) {
        printk("Sending => data send package %li \n ", len);
        print_hex((unsigned const char *)p, len);
        if(interface == NULL){
            return 0;
        }
        if(pcap_sendpacket(interface, p, len) != 0 ){
            printk("pcap_sendpackeet: send failed\n");
            return 0;
        }
    }
    return len;
}

int read_input(void *buf, unsigned long buf_len){
    unsigned char* buffer = buf;
    struct pcap_pkthdr *header;
    const unsigned char *packet;
    int res = pcap_next_ex(interface, &header, &packet);
    if(res == 0){
        return 0;
    }
    for(int i = 0; i < header->caplen; i++){
        *(buffer + i) = *(packet + i);
    }
    
    if(res == 1){
        return header->len;
    }else{
        return 0;
    }
}

int network_interface_init(void){
    char* interface_name = getenv("TAP_INTERFACE_NAME");
    pcap_t *interface_handler = open_interface(interface_name);
    interface = interface_handler;
    if(interface_handler == NULL) {
        log_error("Error opening interface \"%s\"", getenv("TAP_INTERFACE_NAME"));
        exit(EXIT_FAILURE);
    }
    char errbuf[PCAP_ERRBUF_SIZE];
    if (pcap_setnonblock(interface, 1, errbuf) == -1) {
        log_error("Error setting non-blocking mode: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }  
    return pcap_get_selectable_fd(interface_handler);
}

