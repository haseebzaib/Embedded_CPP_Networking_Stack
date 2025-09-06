#include "hal_network.hpp" // Our interface header

// This is the platform-specific part. We include the Npcap header.
#include <pcap.h>

#include <iostream>
#include <vector>

// This handle is the "session" for our opened network device.
static pcap_t* pcap_handle = nullptr;

int hal_net_init(const net::NetworkConfig* config, NetworkFiltering Filtering)
{
    pcap_if_t* all_devices, * device;
    char errbuf[PCAP_ERRBUF_SIZE];
    int i = 0, choice;

    // Use PCAP_SRC_IF_STRING for maximum compatibility.
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &all_devices, errbuf) == -1) {
        std::cerr << "Error in pcap_findalldevs_ex: " << errbuf << std::endl;
        return -1;
    }

    // Print the list of friendly names
    for (device = all_devices; device; device = device->next) {
        std::cout << ++i << ". " << device->name;
        if (device->description) {
            std::cout << " (" << device->description << ")\n";
        }
        else {
            std::cout << " (No description available)\n";
        }
    }

    if (i == 0) {
        std::cout << "No interfaces found! Make sure Npcap is installed." << std::endl;
        return -1;
    }

    std::cout << "Enter the interface number (1-" << i << "): ";
    std::cin >> choice;

    if (choice < 1 || choice > i) {
        std::cout << "Interface number out of range." << std::endl;
        pcap_freealldevs(all_devices);
        return -1;
    }

    // Jump to the selected adapter
    for (device = all_devices, i = 0; i < choice - 1; device = device->next, i++);

    // --- USE THE COMPATIBLE pcap_open() FUNCTION ---
    //
    // Arguments to pcap_open():
    // 1. device->name: The name of the device to open.
    // 2. 65536: snaplen (snapshot length) - max number of bytes to capture per packet. 65536 is standard.
    // 3. PCAP_OPENFLAG_PROMISCUOUS: Flags. We want promiscuous mode.
    // 4. 100: read_timeout in milliseconds. Our more patient 100ms timeout.
    // 5. NULL: Remote authentication, not used here.
    // 6. errbuf: Buffer for error messages.
    pcap_handle = pcap_open(device->name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 100, NULL, errbuf);



    if (pcap_handle == nullptr) {
        std::cerr << "Unable to open the adapter. " << device->name << " is not supported by Npcap/WinPcap or " << errbuf << std::endl;
        return -1;
    }

    // --- SET THE BUFFER SIZE AFTER OPENING ---
    // This requests a 1MB buffer from the driver.
    if (pcap_setbuff(pcap_handle, 1024 * 1024) != 0) {
        std::cerr << "Warning: Could not set buffer size: " << pcap_geterr(pcap_handle) << std::endl;
        // This is not a fatal error, so we can continue.
    }

    std::cout << "HAL Initialized successfully on: " << device->description << std::endl;
    struct bpf_program fp;
    const char filter_exp[10][10] = {"arp"};

    if (pcap_compile(pcap_handle, &fp, filter_exp[(uint8_t)Filtering], 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "couldnt parse filter " << filter_exp[(uint8_t)Filtering] << pcap_geterr(pcap_handle) << std::endl;
        return -1;
    }

    // 2. Apply the compiled filter to our capture handle.
    if (pcap_setfilter(pcap_handle, &fp) == -1) {
        std::cerr << "couldnt install filter" << filter_exp[(uint8_t)Filtering] << pcap_geterr(pcap_handle) << std::endl;
        return -1;
    }

    std::cout << "Successfully applied filter " << filter_exp[(uint8_t)Filtering] << std::endl;
    // ------------------------------------

    pcap_freealldevs(all_devices); // We are done with the device list, free it now.
    return 0;
}

int hal_net_init(const net::NetworkConfig* config) {
    pcap_if_t* all_devices, * device;
    char errbuf[PCAP_ERRBUF_SIZE];
    int i = 0, choice;

    // Use PCAP_SRC_IF_STRING for maximum compatibility.
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &all_devices, errbuf) == -1) {
        std::cerr << "Error in pcap_findalldevs_ex: " << errbuf << std::endl;
        return -1;
    }

    // Print the list of friendly names
    for (device = all_devices; device; device = device->next) {
        std::cout << ++i << ". " << device->name;
        if (device->description) {
            std::cout << " (" << device->description << ")\n";
        }
        else {
            std::cout << " (No description available)\n";
        }
    }

    if (i == 0) {
        std::cout << "No interfaces found! Make sure Npcap is installed." << std::endl;
        return -1;
    }

    std::cout << "Enter the interface number (1-" << i << "): ";
    std::cin >> choice;

    if (choice < 1 || choice > i) {
        std::cout << "Interface number out of range." << std::endl;
        pcap_freealldevs(all_devices);
        return -1;
    }

    // Jump to the selected adapter
    for (device = all_devices, i = 0; i < choice - 1; device = device->next, i++);

    // --- USE THE COMPATIBLE pcap_open() FUNCTION ---
    //
    // Arguments to pcap_open():
    // 1. device->name: The name of the device to open.
    // 2. 65536: snaplen (snapshot length) - max number of bytes to capture per packet. 65536 is standard.
    // 3. PCAP_OPENFLAG_PROMISCUOUS: Flags. We want promiscuous mode.
    // 4. 100: read_timeout in milliseconds. Our more patient 100ms timeout.
    // 5. NULL: Remote authentication, not used here.
    // 6. errbuf: Buffer for error messages.
    pcap_handle = pcap_open(device->name, 65536, PCAP_OPENFLAG_PROMISCUOUS, 100, NULL, errbuf);

    pcap_freealldevs(all_devices); // We are done with the device list, free it now.

    if (pcap_handle == nullptr) {
        std::cerr << "Unable to open the adapter. " << device->name << " is not supported by Npcap/WinPcap or " << errbuf << std::endl;
        return -1;
    }

    // --- SET THE BUFFER SIZE AFTER OPENING ---
    // This requests a 1MB buffer from the driver.
    if (pcap_setbuff(pcap_handle, 1024 * 1024) != 0) {
        std::cerr << "Warning: Could not set buffer size: " << pcap_geterr(pcap_handle) << std::endl;
        // This is not a fatal error, so we can continue.
    }

    std::cout << "HAL Initialized successfully on: " << device->description << std::endl;
    return 0;
}

void hal_net_shutdown() {
    if (pcap_handle) {
        pcap_close(pcap_handle);
    }
}

int hal_net_send(const void* data, size_t length) {
    if (!pcap_handle) return -1;
    if (pcap_sendpacket(pcap_handle, static_cast<const u_char*>(data), length) != 0) {
        std::cerr << "pcap_sendpacket error: " << pcap_geterr(pcap_handle) << std::endl;
        return -1;
    }
    return 0;
}

size_t hal_net_receive(void* buffer, size_t max_length) {
    if (!pcap_handle) return 0;

    struct pcap_pkthdr* header;
    const u_char* packet_data;

    int res = pcap_next_ex(pcap_handle, &header, &packet_data);

    if (res > 0) { // A packet was read
        size_t bytes_to_copy = (header->caplen < max_length) ? header->caplen : max_length;
        memcpy(buffer, packet_data, bytes_to_copy);
        return bytes_to_copy;
    }
    return 0; // Timeout or error
}

