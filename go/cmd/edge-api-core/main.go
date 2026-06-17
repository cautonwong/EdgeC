package main

import (
	"log"
	"net/http"
	"strconv"
	"strings"

	"edgeos.go/pkg/api"
	"edgeos.go/pkg/shm"
	"edgeos.go/pkg/tags"
)

const ServiceName = "edge_api_core"

var matrix *shm.ShadowMatrix

func handleHealth(w http.ResponseWriter, r *http.Request) {
	api.RespondJSON(w, http.StatusOK, 0, "OK", map[string]string{"status": "running"}, ServiceName)
}

func handleGetTag(w http.ResponseWriter, r *http.Request) {
	if matrix == nil {
		api.RespondError(w, http.StatusServiceUnavailable, "DataHub not connected", ServiceName)
		return
	}

	// Simple path routing: /api/v1/hub/tags/{id}
	pathParts := strings.Split(strings.TrimPrefix(r.URL.Path, "/api/v1/hub/tags/"), "/")
	if len(pathParts) == 0 || pathParts[0] == "" {
		// Return all tags - not implemented for demo simplicity
		api.RespondError(w, http.StatusBadRequest, "Missing Tag ID", ServiceName)
		return
	}

	idStr := pathParts[0]
	id, err := strconv.ParseUint(idStr, 10, 32)
	if err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid Tag ID", ServiceName)
		return
	}

	msg, err := matrix.ReadTag(uint32(id))
	if err != nil {
		api.RespondError(w, http.StatusNotFound, "Tag not found", ServiceName)
		return
	}

	valF, valI := msg.Scalar()
	
	// Add metadata if available
	meta := tags.Metadata[uint32(id)]
	
	data := map[string]interface{}{
		"id":    id,
		"name":  meta.Name,
		"unit":  meta.Unit,
		"type":  meta.DataType,
		"value": valF,
		"raw_i": valI,
		"ts":    msg.Timestamp,
	}

	api.RespondJSON(w, http.StatusOK, 0, "OK", data, ServiceName)
}

func main() {
	log.Printf("Starting %s on port 8080...", ServiceName)

	var err error
	// For demo, we try to open "demo" shm with capacity 1024
	matrix, err = shm.OpenShadowMatrix("demo", 1024)
	if err != nil {
		log.Printf("Warning: Failed to open DataHub SHM: %v", err)
	} else {
		defer matrix.Close()
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/health", handleHealth)
	mux.HandleFunc("/api/v1/hub/tags/", handleGetTag)

	// Wrap with middlewares
	recovery := func(next http.Handler) http.Handler {
		return api.RecoveryMiddleware(next, ServiceName)
	}
	handler := api.Chain(mux, api.LoggerMiddleware, recovery)

	server := &http.Server{
		Addr:    ":8080",
		Handler: handler,
	}

	log.Fatal(server.ListenAndServe())
}
