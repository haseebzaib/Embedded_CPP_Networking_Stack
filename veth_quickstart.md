# veth Quickstart (copy/paste)

## 0) Reset lab (safe every time)
```bash
sudo ip link del veth-host 2>/dev/null || true
sudo ip netns del gw 2>/dev/null || true
```

## 1) Create veth pair + bring host side up
```bash
sudo ip link add veth-host type veth peer name veth-peer
sudo ip link set veth-host up
sudo ip link set dev veth-host address f4:7b:09:51:91:63
```

## 2) Create `gw` namespace + configure peer
```bash
sudo ip netns add gw
sudo ip link set veth-peer netns gw
sudo ip netns exec gw ip link set lo up
sudo ip netns exec gw ip link set veth-peer up
sudo ip netns exec gw ip addr add 10.23.42.1/24 dev veth-peer
sudo ip netns exec gw ip link set dev veth-peer address aa:bb:cc:dd:ee:01
```

## 3) Choose mode

### Mode A (ARP discovery; kernel owns 10.23.42.10)
```bash
sudo ip addr add 10.23.42.10/24 dev veth-host
```

### Mode B (IPv4/ICMP honest test; kernel does NOT own 10.23.42.10)
```bash
sudo ip addr del 10.23.42.10/24 dev veth-host 2>/dev/null || true
```

## 4) Run
```bash
sudo env NET_IFACE=veth-host ./build/Networking
```

## 5) Generate traffic from `gw`
```bash
sudo ip netns exec gw ping -c 1 -W 1 10.23.42.10
```

## 6) Fragmentation test (optional)
```bash
sudo ip netns exec gw ip link set dev veth-peer mtu 600
sudo ip netns exec gw ping -c 1 -W 1 -s 1200 10.23.42.10
sudo ip netns exec gw ip link set dev veth-peer mtu 1500
```

