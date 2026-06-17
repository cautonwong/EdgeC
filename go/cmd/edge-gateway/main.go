package main

import (
	"log"
	"net/http"
	"net/http/httputil"
	"net/url"
	"strconv"
	"strings"

	"edgeos.go/pkg/api"
)

const ServiceName = "edge_gateway"

type Service struct {
	Name   string `json:"name"`
	Port   int    `json:"port"`
	Prefix string `json:"prefix"`
	Status string `json:"status"`
}

var registry = []Service{
	{Name: "core_api", Port: 8080, Prefix: "/api/v1/hub/", Status: "online"},
	{Name: "vpp_algo", Port: 8081, Prefix: "/api/v1/algo/", Status: "online"},
	{Name: "ui_host", Port: 8082, Prefix: "/", Status: "online"}, // Fallback for UI assets
}

func handleDiscovery(w http.ResponseWriter, r *http.Request) {
	api.RespondJSON(w, http.StatusOK, 0, "OK", map[string]interface{}{
		"services": registry,
	}, ServiceName)
}

func main() {
	mux := http.NewServeMux()

	// 1. Service Discovery Endpoint
	mux.HandleFunc("/api/v1/discovery", handleDiscovery)

	// 2. Reverse Proxy setup
	proxies := make(map[string]*httputil.ReverseProxy)
	for _, svc := range registry {
		target, err := url.Parse(fmtURL(svc.Port))
		if err != nil {
			log.Fatalf("Failed to parse URL for %s: %v", svc.Name, err)
		}
		proxies[svc.Prefix] = httputil.NewSingleHostReverseProxy(target)
	}

	// 3. Dynamic Router
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		// Add standard headers
		w.Header().Set("X-EdgeOS-Gateway", "1.0")

		// Match longest prefix first
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

	log.Printf("Starting %s on port 8000...", ServiceName)
	// In linux, port 80 requires root. Assuming devcontainer is running as root or has cap_net_bind_service
	if err := server.ListenAndServe(); err != nil {
		log.Fatalf("Gateway failed: %v", err)
	}
}

func fmtURL(port int) string {
	return "http://127.0.0.1:" + strconv.Itoa(port)
}
