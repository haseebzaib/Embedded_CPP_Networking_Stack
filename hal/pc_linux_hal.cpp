// hal/pc_linux_hal.cpp
#include "hal/hal_network.hpp"
#include "hal/hal_logging.hpp"

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>   // ETH_P_ALL
#include <net/if.h>           // ifreq, SIOCGIF*, IFNAMSIZ

namespace {

// --- Module state (PC HAL only; no dynamic allocation) ---
int   g_sock = -1;
int   g_ifindex = 0;
char  g_ifname[IFNAMSIZ] = {};
bool  g_filter_arp_only = false;
uint8_t g_mac[6] = {0};

// tiny htons/ntohs wrappers (we could use the libc ones directly)
inline uint16_t be16(uint16_t x) { return htons(x); }
inline uint16_t from_be16(uint16_t x) { return ntohs(x); }

bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

// Enumerate interfaces using SIOCGIFCONF without heap; pick first UP/RUNNING non-loopback.
bool pick_default_iface(int fd, char out_name[IFNAMSIZ]) {
    // Buffer for list of ifreq entries
    //  * 8KB is plenty for typical systems and avoids heap usage.
    char buf[8192];
    std::memset(buf, 0, sizeof(buf));

    ifconf ifc{};
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
        return false;
    }

    const char* end = buf + ifc.ifc_len;
    for (char* ptr = buf; ptr < end; ) {
        ifreq* ifr = reinterpret_cast<ifreq*>(ptr);

        // Advance ptr to next ifreq entry (portable alignment handling)
#ifdef __linux__
        // On Linux, each entry is fixed-size IFNAMSIZ + sockaddr
        size_t len = sizeof(ifreq);
#else
        size_t len = sizeof(ifreq);
#endif
        ptr += len;

        // Query flags
        ifreq fr_flags{};
        std::strncpy(fr_flags.ifr_name, ifr->ifr_name, IFNAMSIZ - 1);
        if (ioctl(fd, SIOCGIFFLAGS, &fr_flags) < 0) {
            continue;
        }

        const short fl = fr_flags.ifr_flags;
        const bool is_up = (fl & IFF_UP) != 0;
        const bool is_running = (fl & IFF_RUNNING) != 0;
        const bool is_loopback = (fl & IFF_LOOPBACK) != 0;

        if (is_up && is_running && !is_loopback) {
            std::strncpy(out_name, ifr->ifr_name, IFNAMSIZ - 1);
            return true;
        }
    }
    return false;
}

bool resolve_ifindex_mac(int fd, const char* ifname, int& out_ifindex, uint8_t mac[6]) {
    ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    // ifindex
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        NET_LOG_ERROR(HAL, "SIOCGIFINDEX failed");
        return false;
    }
    out_ifindex = ifr.ifr_ifindex;

    // mac
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        NET_LOG_ERROR(HAL, "SIOCGIFHWADDR failed");
        return false;
    }
    std::memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    return true;
}

bool bind_af_packet(int fd, int ifindex) {
    sockaddr_ll sll{};
    sll.sll_family   = AF_PACKET;
    sll.sll_protocol = be16(ETH_P_ALL);
    sll.sll_ifindex  = ifindex;
    return ::bind(fd, reinterpret_cast<sockaddr*>(&sll), sizeof(sll)) == 0;
}

} // namespace

// ------------------ Public HAL API (matches hal_network.hpp) ------------------

int hal_net_init(const net::NetworkConfig* /*config*/, NetworkFiltering filtering)
{
    g_filter_arp_only = (filtering == NetworkFiltering::ARP);

    // 1) Open raw AF_PACKET socket
    g_sock = ::socket(AF_PACKET, SOCK_RAW, be16(ETH_P_ALL));
    if (g_sock < 0) {
        NET_LOG_ERROR(HAL, "socket(AF_PACKET) failed");
        return -1;
    }

    // 2) Pick interface: use NET_IFACE env, else first UP/RUNNING non-loopback
    const char* env = std::getenv("NET_IFACE");
    if (env && *env) {
        std::strncpy(g_ifname, env, IFNAMSIZ - 1);
    } else {
        if (!pick_default_iface(g_sock, g_ifname)) {
            NET_LOG_ERROR(HAL, "No suitable interface found; set NET_IFACE");
            ::close(g_sock); g_sock = -1;
            return -1;
        }
    }

    // 3) Resolve ifindex + MAC
    if (!resolve_ifindex_mac(g_sock, g_ifname, g_ifindex, g_mac)) {
        ::close(g_sock); g_sock = -1;
        return -1;
    }

    // 4) Bind and make non-blocking
    if (!bind_af_packet(g_sock, g_ifindex)) {
        NET_LOG_ERROR(HAL, "bind(AF_PACKET) failed on %s", g_ifname);
        ::close(g_sock); g_sock = -1;
        return -1;
    }
    (void)set_nonblocking(g_sock);

    NET_LOG_INFO(HAL, "HAL init on iface %s (ifindex %d)", g_ifname, g_ifindex);
    return 0;
}

// Overload without filtering argument (kept for your API)
int hal_net_init(const net::NetworkConfig* config)
{
    return hal_net_init(config, NetworkFiltering::ARP);
}

void hal_net_shutdown()
{
    if (g_sock >= 0) {
        ::close(g_sock);
    }
    g_sock = -1;
    g_ifindex = 0;
    std::memset(g_ifname, 0, sizeof(g_ifname));
    std::memset(g_mac, 0, sizeof(g_mac));
    g_filter_arp_only = false;
}

int hal_net_send(const void* data, size_t length)
{
    if (g_sock < 0 || data == nullptr || length == 0) return -1;
    // Ethernet header (dst/src/type) is already in 'data' — just send it.
    ssize_t n = ::send(g_sock, data, length, 0);
    if (n != static_cast<ssize_t>(length)) {
        NET_LOG_ERROR(HAL, "send() failed (%zd/%zu)", n, length);
        return -1;
    }
    return 0;
}

size_t hal_net_receive(void* buffer, size_t max_length)
{
    if (g_sock < 0 || buffer == nullptr || max_length == 0) return 0;

    // Non-blocking read of a single frame
    ssize_t n = ::recv(g_sock, buffer, max_length, MSG_DONTWAIT);
    if (n <= 0) {
        return 0; // nothing available or EAGAIN; caller polls again
    }

    // Optional software filter: drop non-ARP frames
    if (g_filter_arp_only && static_cast<size_t>(n) >= 14) {
        const uint8_t* p = static_cast<const uint8_t*>(buffer);
        // ethertype = bytes 12..13
        uint16_t ethertype_be = (static_cast<uint16_t>(p[12]) << 8) | p[13];
     if (ethertype_be != 0x0806) {   // compare directly
        return 0;
    }

    }

    return static_cast<size_t>(n);
}
