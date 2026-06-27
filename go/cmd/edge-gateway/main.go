package main

import (
	"encoding/json"
	"fmt"
	"log"
	"math/rand"
	"net/http"
	"net/http/httputil"
	"net/url"
		"strings"
	"sync"
	"time"

	"edgeos.go/pkg/api"
)

const ServiceName = "edge_gateway"

type Service struct {
	Name   string `json:"name"`
	Port   int    `json:"port"`
	Prefix string `json:"prefix"`
	Status string `json:"status"`
}

var (
	registryLock sync.RWMutex
	registry     = []Service{
		{Name: "core_api", Port: 8080, Prefix: "/api/v1/hub/", Status: "online"},
		{Name: "vpp_algo", Port: 8081, Prefix: "/api/v1/algo/", Status: "online"},
		{Name: "c_rest", Port: 9091, Prefix: "/api/v1/", Status: "online"},
		{Name: "ui_host", Port: 8082, Prefix: "/", Status: "online"}, // Fallback for UI assets
	}
	proxies = make(map[string]*httputil.ReverseProxy)
)

func handleDiscovery(w http.ResponseWriter, r *http.Request) {
	registryLock.RLock()
	defer registryLock.RUnlock()
	
	api.RespondJSON(w, http.StatusOK, 0, "OK", map[string]interface{}{
		"services": registry,
	}, ServiceName)
}

func handleSystemStatsMock(w http.ResponseWriter, r *http.Request) {
	buses := make([]map[string]interface{}, 32)
	for i := 0; i < 32; i++ {
		busy := rand.Float32() < 0.25
		queue := 0
		if busy {
			queue = rand.Intn(5) + 1
		}
		buses[i] = map[string]interface{}{
			"id":    i,
			"busy":  busy,
			"queue": queue,
		}
	}

	api.RespondJSON(w, http.StatusOK, 0, "OK", map[string]interface{}{
		"uptime_sec_proof": time.Now().Unix() % 10000,
		"engine_status":    "ALIVE_AND_KICKING",
		"datahub": map[string]interface{}{
			"total_writes":   rand.Intn(100000),
			"listener_count": 3,
		},
		"buses": buses,
	}, ServiceName)
}

// 🟢 [TSK-01] 动态服务注册处理函数 (Thread-safe)
func handleRegisterService(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	var newSvc Service
	if err := json.NewDecoder(r.Body).Decode(&newSvc); err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid request payload", ServiceName)
		return
	}

	if newSvc.Name == "" || newSvc.Port <= 0 || newSvc.Prefix == "" {
		api.RespondError(w, http.StatusBadRequest, "Missing required service parameters", ServiceName)
		return
	}

	// 强制校验前缀规则，防止恶意劫持核心路由
	if !strings.HasPrefix(newSvc.Prefix, "/") || !strings.HasSuffix(newSvc.Prefix, "/") {
		api.RespondError(w, http.StatusBadRequest, "Prefix must start and end with /", ServiceName)
		return
	}

	targetURL, err := url.Parse(fmtURL(newSvc.Port))
	if err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid target port address", ServiceName)
		return
	}

	registryLock.Lock()
	defer registryLock.Unlock()

	// 检查是否已存在同名前缀或同名服务，若存在则更新
	exists := false
	for i, svc := range registry {
		if svc.Prefix == newSvc.Prefix || svc.Name == newSvc.Name {
			registry[i] = newSvc
			exists = true
			break
		}
	}

	if !exists {
		// 新增服务注册，由于 / 是 fallback，必须保证 / 在数组的最后，以便最长匹配正确运作
		// 我们将新服务插入到 / 之前
		idx := len(registry) - 1
		registry = append(registry[:idx], append([]Service{newSvc}, registry[idx:]...)...)
	}

	// 动态创建并注册反向代理实例
	proxies[newSvc.Prefix] = httputil.NewSingleHostReverseProxy(targetURL)
	newSvc.Status = "online"

	log.Printf("[Mesh-Registry] Dynamic service registered: %s (Port: %d, Prefix: %s)", newSvc.Name, newSvc.Port, newSvc.Prefix)
	api.RespondJSON(w, http.StatusOK, 0, "Service registered successfully", newSvc, ServiceName)
}

func main() {
	mux := http.NewServeMux()

	// 1. Service Discovery & Registry Endpoints
	mux.HandleFunc("/api/v1/discovery", handleDiscovery)
	mux.HandleFunc("/api/v1/system/stats", handleSystemStatsMock)
	mux.HandleFunc("/api/v1/internal/registry", handleRegisterService)

	// 2. Initial Reverse Proxy setup
	registryLock.Lock()
	for _, svc := range registry {
		target, err := url.Parse(fmtURL(svc.Port))
		if err != nil {
			log.Fatalf("Failed to parse URL for %s: %v", svc.Name, err)
		}
		proxies[svc.Prefix] = httputil.NewSingleHostReverseProxy(target)
	}
	registryLock.Unlock()

	// 3. Dynamic Router (最长前缀匹配)
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("X-EdgeOS-Gateway", "2.0-Mesh")

		registryLock.RLock()
		defer registryLock.RUnlock()

		var bestMatch *httputil.ReverseProxy
		bestLen := -1

		for prefix, proxy := range proxies {
			if strings.HasPrefix(r.URL.Path, prefix) && len(prefix) > bestLen {
				bestMatch = proxy
				bestLen = len(prefix)
			}
		}

		if bestMatch != nil {
			bestMatch.ServeHTTP(w, r)
		} else {
			api.RespondError(w, http.StatusNotFound, "Route not found in Gateway", ServiceName)
		}
	})

	// Wrap with middlewares
	recovery := func(next http.Handler) http.Handler {
		return api.RecoveryMiddleware(next, ServiceName)
	}
	handler := api.Chain(mux, api.LoggerMiddleware, recovery)

	server := &http.Server{
		Addr:    ":8000",
		Handler: handler,
	}

	log.Printf("Starting %s (with Mesh Routing) on port 8000...", ServiceName)
	if err := server.ListenAndServe(); err != nil {
		log.Fatalf("Gateway failed: %v", err)
	}
}

func fmtURL(port int) string {
	return fmt.Sprintf("http://127.0.0.1:%d", port)
}
