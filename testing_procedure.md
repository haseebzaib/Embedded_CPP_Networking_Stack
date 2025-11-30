# Testing Procedure (ARP Gateway Discovery)

This document records a repeatable way to test the ARP flow using a local virtual NIC and a separate network namespace on Linux. It assumes no heap use in the core stack and uses the existing `Networking` executable.

## Overview
- Create a veth pair to simulate a LAN: `veth-host` (host side) and `veth-peer` (gateway side).
- Put `veth-peer` into a dedicated namespace `gw` so it behaves like a separate machine.
- Assign IPs: host = `10.23.42.10/24`, gateway = `10.23.42.1/24`.
- Run `tcpdump` in the `gw` namespace to watch ARP traffic.
- Run `Networking` on the host side, targeting the gateway IP, and observe ARP request/reply.

## Setup (one-time per boot)

### 1. Host side (veth-host)
```bash
# Clean up any old veth pair (ignore errors)
sudo ip link del veth-host 2>/dev/null || true

# Create veth pair
sudo ip link add veth-host type veth peer name veth-peer

# Configure host side
sudo ip addr add 10.23.42.10/24 dev veth-host
sudo ip link set veth-host up
sudo ip link set dev veth-host address f4:7b:09:51:91:63   # matches NetworkConfig
```

### 2. Gateway side (namespace `gw`)
```bash
# Create a separate namespace to act as the gateway
sudo ip netns add gw

# Move peer into the gw namespace
sudo ip link set veth-peer netns gw

# Inside the gw namespace, bring interfaces up and set IP/MAC
sudo ip netns exec gw ip link set lo up
sudo ip netns exec gw ip addr add 10.23.42.1/24 dev veth-peer
sudo ip netns exec gw ip link set veth-peer up
sudo ip netns exec gw ip link set dev veth-peer address aa:bb:cc:dd:ee:01
```

## Build
From repo root:
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```
Binary: `build/Networking`

## Observe traffic
Terminal 1 (watch ARP on the peer/gateway side, inside `gw`):
```bash
sudo ip netns exec gw tcpdump -n -e -vvv -i veth-peer arp
```

## Run the app
Terminal 2 (repo root):
```bash
# If needed, set capabilities once:
# sudo setcap cap_net_raw,cap_net_admin=eip build/Networking

NET_IFACE=veth-host ./build/Networking
```

Expectations:
- App logs show ARP requests and “gateway MAC resolved” once a reply is received.
- `tcpdump` shows the broadcast ARP request from `veth-host` and the unicast ARP reply from `veth-peer` (MAC `aa:bb:cc:dd:ee:01`).
- Typical app logs for a successful run:
  - `[NET] [DEBUG] poll() received a frame of size: 42`
  - `[NET] [DEBUG] Frame has EtherType 0x0806`
  - `[ARP] [DEBUG] OP-CODE RECV: 2`
  - `[NET] [DEBUG] MAC Address: aa:bb:cc:dd:ee:1`
  - `[HAL] [INFO] SUCCESS: Gateway MAC address has been resolved!`

## Cleanup (optional)
```bash
sudo ip link del veth-host
sudo ip netns del gw
```
This deletes both ends of the veth pair and removes the `gw` namespace.

## Notes
- Ensure `NetworkingStack.cpp` `NetworkConfig` matches the IP/MAC you assign to `veth-host` and the gateway IP on `veth-peer`:
  - `ipv4_address = {10,23,42,10}`
  - `gateway_address = {10,23,42,1}`
  - `mac_address = {0xF4,0x7B,0x09,0x51,0x91,0x63}`
- The HAL filters to ARP if `NetworkFiltering::ARP` is passed; this is fine for this test.
- No heap is used in the portable core; the PC logging HAL uses `std::string`/`std::map`, which is acceptable for this host-only test.

