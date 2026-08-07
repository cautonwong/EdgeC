package main

import (
	"log"
	"net/http"
	"os"

	"edgeos.go/pkg/api"
	"edgeos.go/pkg/service"
)

func main() {
	// 1. Create a new service instance
	svc := service.New("vpp_algo", 8081)

	// 2. Set API key from environment variable
	apiKey := os.Getenv("DEMO_ALGO_API_KEY")
	if apiKey == "" {
		if os.Getenv("EDGE_ENV") == "production" || os.Getenv("EDGE_ENV") == "prod" {
			log.Fatalf("[FATAL] Security Redline: DEMO_ALGO_API_KEY must be set in production mode!")
		}
		log.Println("[WARN] DEMO_ALGO_API_KEY missing, using dev fallback key")
		apiKey = "TEST_ONLY_demo_algo_key"
	}
	svc.SetAPIKey(apiKey)

	// 3. Register endpoints
	svc.Handle("/api/v1/algo/state", func(w http.ResponseWriter, r *http.Request) {
		state := map[string]interface{}{
			"status": "calculating",
			"target": 5000, // Target power in Watts
		}
		api.RespondJSON(w, http.StatusOK, 0, "OK", state, svc.Name)
	})

	svc.Handle("/api/v1/algo/control", func(w http.ResponseWriter, r *http.Request) {
		// Only accessible via POST with correct X-API-KEY
		api.RespondJSON(w, http.StatusOK, 0, "Control command accepted", nil, svc.Name)
	})

	// 4. Run the service
	if err := svc.Run(); err != nil {
		log.Fatalf("Service stopped: %v", err)
	}
}
