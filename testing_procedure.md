# Testing Procedure (veth Lab: ARP + IPv4)

This document records a repeatable way to test the stack using a local veth pair and a separate network namespace on Linux.

It covers:
- ARP gateway discovery (original flow)
- IPv4 receive parsing + fragmentation detection (current work)

## Overview
- veth pair simulates a LAN: `veth-host` (where the stack runs) and `veth-peer` (the “gateway” in a namespace).
- `gw` network namespace behaves like a separate machine.
- The stack uses `NetworkConfig`:
  - stack IP: `10.23.42.10`
  - gateway IP: `10.23.42.1`
  - stack MAC: `f4:7b:09:51:91:63`

## Setup (repeatable)

### 0. Clean up previous lab (safe to run every time)
```bash
sudo ip link del veth-host 2>/dev/null || true
sudo ip netns del gw 2>/dev/null || true
```

### 1. Create veth pair
```bash
# Create veth pair
sudo ip link add veth-host type veth peer name veth-peer
sudo ip link set veth-host up
```

### 2. Create gateway namespace + configure veth-peer
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

## Mode A: ARP gateway discovery (kernel also has the host IP)
This matches the original ARP discovery flow. In this mode the Linux kernel owns `10.23.42.10`, which is fine for ARP discovery, but it will also answer ICMP/etc later.

### A1. Configure veth-host (with host IP)
```bash
sudo ip addr add 10.23.42.10/24 dev veth-host
sudo ip link set dev veth-host address f4:7b:09:51:91:63   # must match NetworkConfig
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

### A2. Observe traffic
Terminal 1 (watch ARP on the peer/gateway side, inside `gw`):
```bash
sudo ip netns exec gw tcpdump -n -e -vvv -i veth-peer arp
```

### A3. Run the app
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

## Mode B: IPv4 receive parsing (stack “owns” the IP, kernel does not)
Use this mode when working on IPv4/ICMP so the kernel doesn’t “help” by replying.

### B1. Configure veth-host (NO host IP)
```bash
# Make sure kernel is NOT assigned the stack IP:
sudo ip addr del 10.23.42.10/24 dev veth-host 2>/dev/null || true

# Still set MAC to match NetworkConfig (so peers ARP correctly)
sudo ip link set dev veth-host address f4:7b:09:51:91:63
```

### B2. Make sure HAL filtering allows IPv4
If your stack is still initializing HAL with ARP-only filtering, IPv4 frames will never reach `handle_ipv4_frame`.

Set it to the “no filtering” mode (example: `NetworkFiltering::NONE`) in `NetworkingStack.cpp` when doing IPv4/ICMP work.

### B3. Observe traffic (ARP + ICMP)
Terminal 1:
```bash
sudo ip netns exec gw tcpdump -n -e -vvv -i veth-peer arp or icmp
```

### B4. Run the app
Terminal 2:
```bash
NET_IFACE=veth-host ./build/Networking
```

### B5. Send IPv4 traffic toward the stack IP
Terminal 3:
```bash
sudo ip netns exec gw ping -c 1 -W 1 10.23.42.10
```

Expected:
- `ping` times out (until you implement ICMP echo reply).
- Stack logs show IPv4 frames being received and parsed.

## Fragmentation test (for “drop fragments” logic)
Force fragmentation by lowering the MTU in `gw` and sending a large packet.

```bash
sudo ip netns exec gw ip link set dev veth-peer mtu 600
sudo ip netns exec gw ping -c 1 -W 1 -s 1200 10.23.42.10
```

Expected:
- Your IPv4 parsing logs should show `MF=1` and/or non-zero fragment offset, and your stack should drop the packet.

Restore MTU:
```bash
sudo ip netns exec gw ip link set dev veth-peer mtu 1500
```

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
- The HAL filters to ARP if `NetworkFiltering::ARP` is passed; this is fine for Mode A, but will block IPv4 in Mode B.
- No heap is used in the portable core; the PC logging HAL uses `std::string`/`std::map`, which is acceptable for this host-only test.
