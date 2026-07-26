package p2p

import (
	"encoding/json"
	"fmt"
	"net"
	"sync"
	"time"
)

type PeerNode struct {
	NodeID    string    `json:"node_id"`
	Product   string    `json:"product"`
	IP        string    `json:"ip"`
	Port      int       `json:"port"`
	LastSeen  time.Time `json:"last_seen"`
	Battery   float64   `json:"battery"`
	NetStatus string    `json:"net_status"`
}

type PeerDiscovery struct {
	nodeID      string
	product     string
	port        int
	listenAddr  string
	udpConn     *net.UDPConn
	peers       map[string]*PeerNode
	mu          sync.RWMutex
	stopChan    chan struct{}
	broadcastMs time.Duration
}

func NewPeerDiscovery(nodeID, product string, port int) *PeerDiscovery {
	return &PeerDiscovery{
		nodeID:      nodeID,
		product:     product,
		port:        port,
		listenAddr:  "239.255.255.250:9999", // Multicast / UDP LAN discovery group
		peers:       make(map[string]*PeerNode),
		stopChan:    make(chan struct{}),
		broadcastMs: 2 * time.Second,
	}
}

func (pd *PeerDiscovery) Start() error {
	addr, err := net.ResolveUDPAddr("udp", pd.listenAddr)
	if err != nil {
		return fmt.Errorf("resolve UDP addr failed: %w", err)
	}

	conn, err := net.ListenMulticastUDP("udp", nil, addr)
	if err != nil {
		// Fallback to UDP broadcast/unicast listener if multicast fails
		conn, err = net.ListenUDP("udp", &net.UDPAddr{Port: 9999})
		if err != nil {
			return fmt.Errorf("listen UDP failed: %w", err)
		}
	}
	pd.udpConn = conn

	go pd.listenLoop()
	go pd.broadcastLoop()
	go pd.cleanupLoop()

	return nil
}

func (pd *PeerDiscovery) Stop() {
	close(pd.stopChan)
	if pd.udpConn != nil {
		pd.udpConn.Close()
	}
}

func (pd *PeerDiscovery) broadcastLoop() {
	ticker := time.NewTicker(pd.broadcastMs)
	defer ticker.Stop()

	targetAddr, err := net.ResolveUDPAddr("udp", pd.listenAddr)
	if err != nil {
		return
	}

	for {
		select {
		case <-pd.stopChan:
			return
		case <-ticker.C:
			nodeMsg := PeerNode{
				NodeID:    pd.nodeID,
				Product:   pd.product,
				IP:        getOutboundIP(),
				Port:      pd.port,
				LastSeen:  time.Now(),
				Battery:   100.0,
				NetStatus: "LAN_P2P_ONLINE",
			}
			data, err := json.Marshal(nodeMsg)
			if err == nil && pd.udpConn != nil {
				pd.udpConn.WriteToUDP(data, targetAddr)
			}
		}
	}
}

func (pd *PeerDiscovery) listenLoop() {
	buf := make([]byte, 2048)
	for {
		select {
		case <-pd.stopChan:
			return
		default:
			if pd.udpConn == nil {
				return
			}
			pd.udpConn.SetReadDeadline(time.Now().Add(1 * time.Second))
			n, src, err := pd.udpConn.ReadFromUDP(buf)
			if err != nil {
				continue
			}

			var node PeerNode
			if err := json.Unmarshal(buf[:n], &node); err != nil {
				continue
			}

			// Ignore self announcements
			if node.NodeID == pd.nodeID {
				continue
			}

			if node.IP == "" && src != nil {
				node.IP = src.IP.String()
			}
			node.LastSeen = time.Now()

			pd.mu.Lock()
			pd.peers[node.NodeID] = &node
			pd.mu.Unlock()
		}
	}
}

func (pd *PeerDiscovery) cleanupLoop() {
	ticker := time.NewTicker(5 * time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-pd.stopChan:
			return
		case <-ticker.C:
			pd.mu.Lock()
			now := time.Now()
			for id, peer := range pd.peers {
				if now.Sub(peer.LastSeen) > 10*time.Second {
					delete(pd.peers, id)
				}
			}
			pd.mu.Unlock()
		}
	}
}

func (pd *PeerDiscovery) GetActivePeers() []PeerNode {
	pd.mu.RLock()
	defer pd.mu.RUnlock()

	result := make([]PeerNode, 0, len(pd.peers))
	for _, p := range pd.peers {
		result = append(result, *p)
	}
	return result
}

func getOutboundIP() string {
	conn, err := net.Dial("udp", "8.8.8.8:80")
	if err != nil {
		return "127.0.0.1"
	}
	defer conn.Close()
	localAddr := conn.LocalAddr().(*net.UDPAddr)
	return localAddr.IP.String()
}
