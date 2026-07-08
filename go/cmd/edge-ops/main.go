package main

import (
	"encoding/json"
	"log"
	"math/rand"
	"net/http"
	"strings"
	"sync"
	"time"

	"edgeos.go/pkg/api"
	"edgeos.go/pkg/service"
)

const ServiceName = "edge_ops"

// DeviceInfo represents an edge agent device
type DeviceInfo struct {
	DeviceID       string `json:"device_id"`
	ProductType    string `json:"product_type"`
	Online         bool   `json:"online"`
	LastHeartbeat  int64  `json:"last_heartbeat"`
	RingWatermark  uint64 `json:"ring_watermark"`
}

var (
	devices     = make(map[string]*DeviceInfo)
	devicesLock sync.RWMutex
)

// heartbeatPayload is the expected MQTT heartbeat message structure
type heartbeatPayload struct {
	DeviceID      string `json:"device_id"`
	ProductType   string `json:"product_type"`
	RingWatermark uint64 `json:"ring_watermark"`
}

func handleDeviceList(w http.ResponseWriter, r *http.Request) {
	devicesLock.RLock()
	list := make([]*DeviceInfo, 0, len(devices))
	for _, d := range devices {
		list = append(list, d)
	}
	devicesLock.RUnlock()

	api.RespondJSON(w, http.StatusOK, 0, "OK", list, ServiceName)
}

func handleDeviceDetail(w http.ResponseWriter, r *http.Request) {
	// Path: /api/v1/devices/{id}
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/api/v1/devices/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		api.RespondError(w, http.StatusBadRequest, "Missing device ID", ServiceName)
		return
	}

	id := parts[0]
	devicesLock.RLock()
	dev, ok := devices[id]
	devicesLock.RUnlock()

	if !ok {
		api.RespondError(w, http.StatusNotFound, "Device not found", ServiceName)
		return
	}

	api.RespondJSON(w, http.StatusOK, 0, "OK", dev, ServiceName)
}

// processHeartbeat processes a heartbeat message from an edge agent
func processHeartbeat(payload []byte) {
	var hb heartbeatPayload
	if err := json.Unmarshal(payload, &hb); err != nil {
		log.Printf("[edge_ops] Invalid heartbeat payload: %v", err)
		return
	}

	if hb.DeviceID == "" {
		log.Printf("[edge_ops] Heartbeat missing device_id")
		return
	}

	now := time.Now().Unix()
	devicesLock.Lock()
	dev, exists := devices[hb.DeviceID]
	if !exists {
		dev = &DeviceInfo{
			DeviceID:    hb.DeviceID,
			ProductType: hb.ProductType,
		}
		devices[hb.DeviceID] = dev
	}
	dev.Online = true
	dev.LastHeartbeat = now
	if hb.RingWatermark > 0 {
		dev.RingWatermark = hb.RingWatermark
	}
	if hb.ProductType != "" {
		dev.ProductType = hb.ProductType
	}
	devicesLock.Unlock()

	log.Printf("[edge_ops] Heartbeat received from %s (product: %s)", hb.DeviceID, hb.ProductType)
}

// offlineChecker periodically marks devices as offline if no heartbeat within 60s
func offlineChecker(ctx *service.EdgeService) {
	ticker := time.NewTicker(30 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		now := time.Now().Unix()
		devicesLock.Lock()
		for id, dev := range devices {
			if dev.Online && (now-dev.LastHeartbeat) > 60 {
				dev.Online = false
				log.Printf("[edge_ops] Device %s marked offline (no heartbeat for >60s)", id)
			}
		}
		devicesLock.Unlock()
	}
}

// simulateMQTT generates periodic mock heartbeats when real MQTT is unavailable
func simulateMQTT(ctx *service.EdgeService) {
	agentIDs := []string{"AGENT-A001", "AGENT-A002", "AGENT-A003", "AGENT-B001", "AGENT-C001"}
	productTypes := []string{"iems", "ems", "edge-gateway", "vpp"}

	ticker := time.NewTicker(15 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		for _, id := range agentIDs {
			hb := heartbeatPayload{
				DeviceID:      id,
				ProductType:   productTypes[rand.Intn(len(productTypes))],
				RingWatermark: uint64(rand.Intn(1000)),
			}
			data, _ := json.Marshal(hb)
			processHeartbeat(data)
		}
	}
}

func main() {
	svc := service.New(ServiceName, 8100)

	// API v1 routes
	svc.Handle("/api/v1/devices", handleDeviceList)
	svc.Handle("/api/v1/devices/", handleDeviceDetail)

	// Start background goroutines
	go simulateMQTT(svc)
	go offlineChecker(svc)

	log.Printf("[edge_ops] Starting MQTT simulation with 5 agent devices...")
	log.Printf("[edge_ops] REST API available at :8100")
	log.Printf("[edge_ops] Endpoints:")
	log.Printf("  GET /health")
	log.Printf("  GET /api/v1/devices")
	log.Printf("  GET /api/v1/devices/{id}")

	if err := svc.Run(); err != nil {
		log.Fatalf("[edge_ops] Server failed: %v", err)
	}
}
