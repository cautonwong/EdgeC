package main

import (
	"fmt"
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

func handleStream(w http.ResponseWriter, r *http.Request) {
	if matrix == nil {
		api.RespondError(w, http.StatusServiceUnavailable, "DataHub not connected", ServiceName)
		return
	}

	flusher, ok := w.(http.Flusher)
	if !ok {
		api.RespondError(w, http.StatusInternalServerError, "Streaming unsupported", ServiceName)
		return
	}

	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")

	// Parse requested tags from query, or default to some
	// For demo, we just watch 1001, 1002, 1003, 1004
	tagIDs := []uint32{1001, 1002, 1003, 1004}

	ch := make(chan *shm.PipeMsg, 100)
	ctx := r.Context()
	
	go matrix.Watch(ctx, tagIDs, ch)

	for {
		select {
		case <-ctx.Done():
			return
		case msg := <-ch:
			valF, _ := msg.Scalar()
			meta := tags.Metadata[msg.TagID]
			
			// Format as SSE
			// Example JSON payload
			// data: {"id":1001,"value":220.88,"ts":12345}
			
			fmt.Fprintf(w, "event: update\n")
			fmt.Fprintf(w, "data: {\"id\":%d,\"name\":\"%s\",\"value\":%f,\"ts\":%d}\n\n",
				msg.TagID, meta.Name, valF, msg.Timestamp)
			flusher.Flush()
		}
	}
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
	mux.HandleFunc("/api/v1/hub/stream", handleStream)
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
