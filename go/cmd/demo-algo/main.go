package main

import (
	"log"
	"net/http"

	"edgeos.go/pkg/api"
	"edgeos.go/pkg/service"
)

func main() {
	// 1. Create a new service instance
	svc := service.New("vpp_algo", 8081)

	// 2. Set API key for secure endpoints
	svc.SetAPIKey("secret-token-123")

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
