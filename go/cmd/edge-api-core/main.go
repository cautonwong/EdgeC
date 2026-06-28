package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"math"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"
	"time"
	"unsafe"

	"edgeos.go/pkg/api"
	"edgeos.go/pkg/shm"
	"edgeos.go/pkg/tags"
)

const ServiceName = "edge_api_core"

var (
	matrix         *shm.ShadowMatrix
	mockLock       sync.RWMutex
	mockEnabled    bool
	mockValues     = make(map[uint32]float64)
	mockWatchers   = make([]chan *shm.PipeMsg, 0)
	watchersLock   sync.Mutex
	mockCancel      context.CancelFunc
	currentLogLevel = "info"
	logLevelMutex   sync.RWMutex
)

func handleHealth(w http.ResponseWriter, r *http.Request) {
	api.RespondJSON(w, http.StatusOK, 0, "OK", map[string]string{"status": "running"}, ServiceName)
}

func handleGetTag(w http.ResponseWriter, r *http.Request) {
	// Simple path routing: /api/v1/hub/tags/{id}
	pathParts := strings.Split(strings.TrimPrefix(r.URL.Path, "/api/v1/hub/tags/"), "/")
	if len(pathParts) == 0 || pathParts[0] == "" {
		api.RespondError(w, http.StatusBadRequest, "Missing Tag ID", ServiceName)
		return
	}

	idStr := pathParts[0]
	id, err := strconv.ParseUint(idStr, 10, 32)
	if err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid Tag ID", ServiceName)
		return
	}

	mockLock.RLock()
	isMocked := mockEnabled
	var mockVal float64
	if isMocked {
		mockVal = mockValues[uint32(id)]
	}
	mockLock.RUnlock()

	var msg *shm.PipeMsg

	if isMocked {
		msg = &shm.PipeMsg{
			TagID:     uint32(id),
			Timestamp: uint64(time.Now().UnixNano() / 1e6),
		}
		*(*float64)(unsafe.Pointer(&msg.Payload[0])) = mockVal
	} else {
		if matrix == nil {
			api.RespondError(w, http.StatusServiceUnavailable, "DataHub not connected", ServiceName)
			return
		}
		msg, err = matrix.ReadTag(uint32(id))
		if err != nil {
			api.RespondError(w, http.StatusNotFound, "Tag not found", ServiceName)
			return
		}
	}

	valF, valI := msg.Scalar()
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
	flusher, ok := w.(http.Flusher)
	if !ok {
		api.RespondError(w, http.StatusInternalServerError, "Streaming unsupported", ServiceName)
		return
	}

	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")

	tagIDs := []uint32{1001, 1002, 1003, 1004}
	ch := make(chan *shm.PipeMsg, 100)
	ctx := r.Context()

	mockLock.RLock()
	isMocked := mockEnabled
	mockLock.RUnlock()

	if isMocked {
		watchersLock.Lock()
		mockWatchers = append(mockWatchers, ch)
		watchersLock.Unlock()

		defer func() {
			watchersLock.Lock()
			for i, watcher := range mockWatchers {
				if watcher == ch {
					mockWatchers = append(mockWatchers[:i], mockWatchers[i+1:]...)
					break
				}
			}
			watchersLock.Unlock()
		}()
	} else {
		if matrix == nil {
			api.RespondError(w, http.StatusServiceUnavailable, "DataHub not connected", ServiceName)
			return
		}
		go matrix.Watch(ctx, tagIDs, ch)
	}

	for {
		select {
		case <-ctx.Done():
			return
		case msg := <-ch:
			valF, _ := msg.Scalar()
			meta := tags.Metadata[msg.TagID]

			fmt.Fprintf(w, "event: update\n")
			fmt.Fprintf(w, "data: {\"id\":%d,\"name\":\"%s\",\"value\":%f,\"ts\":%d}\n\n",
				msg.TagID, meta.Name, valF, msg.Timestamp)
			flusher.Flush()
		}
	}
}

type MockToggleRequest struct {
	Enabled bool `json:"enabled"`
}

func handleMockToggle(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	var req MockToggleRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid request payload", ServiceName)
		return
	}

	mockLock.Lock()
	if req.Enabled && !mockEnabled {
		mockEnabled = true
		var ctx context.Context
		ctx, mockCancel = context.WithCancel(context.Background())
		go runMockSimulation(ctx)
	} else if !req.Enabled && mockEnabled {
		mockEnabled = false
		if mockCancel != nil {
			mockCancel()
			mockCancel = nil
		}
	}
	mockLock.Unlock()

	api.RespondJSON(w, http.StatusOK, 0, "Mock state toggled successfully", map[string]bool{"mock_enabled": req.Enabled}, ServiceName)
}

func runMockSimulation(ctx context.Context) {
	ticker := time.NewTicker(100 * time.Millisecond)
	defer ticker.Stop()

	start := time.Now()
	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			mockLock.Lock()
			if !mockEnabled {
				mockLock.Unlock()
				return
			}
			t := time.Since(start).Seconds()

			mockValues[1001] = 220.0 + 5.0*math.Sin(2.0*math.Pi*t/10.0)
			mockValues[1002] = 5.0 + 0.5*math.Sin(2.0*math.Pi*t/15.0)
			mockValues[1003] = mockValues[1001] * mockValues[1002]
			mockValues[1004] = 50.0 + 0.02*math.Sin(2.0*math.Pi*t/5.0)

			now := uint64(time.Now().UnixNano() / 1e6)
			watchersLock.Lock()
			for _, ch := range mockWatchers {
				for _, id := range []uint32{1001, 1002, 1003, 1004} {
					val := mockValues[id]
					msg := &shm.PipeMsg{
						TagID:     id,
						Timestamp: now,
					}
					*(*float64)(unsafe.Pointer(&msg.Payload[0])) = val
					select {
					case ch <- msg:
					default:
					}
				}
			}
			watchersLock.Unlock()
			mockLock.Unlock()
		}
	}
}

type BatchReadRequest struct {
	IDs []uint32 `json:"ids"`
}

type TagValueData struct {
	ID    uint32  `json:"id"`
	Name  string  `json:"name"`
	Unit  string  `json:"unit"`
	Type  string  `json:"type"`
	Value float64 `json:"value"`
	TS    uint64  `json:"ts"`
}

func handleBatchRead(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	var req BatchReadRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid request payload", ServiceName)
		return
	}

	mockLock.RLock()
	isMocked := mockEnabled
	mockLock.RUnlock()

	results := make(map[string]TagValueData)
	now := uint64(time.Now().UnixNano() / 1e6)

	for _, id := range req.IDs {
		var val float64
		var ts uint64
		var err error

		if isMocked {
			mockLock.RLock()
			val = mockValues[id]
			mockLock.RUnlock()
			ts = now
		} else {
			if matrix == nil {
				api.RespondError(w, http.StatusServiceUnavailable, "DataHub not connected", ServiceName)
				return
			}
			var msg *shm.PipeMsg
			msg, err = matrix.ReadTag(id)
			if err != nil {
				continue
			}
			val, _ = msg.Scalar()
			ts = msg.Timestamp
		}

		meta := tags.Metadata[id]
		results[strconv.Itoa(int(id))] = TagValueData{
			ID:    id,
			Name:  meta.Name,
			Unit:  meta.Unit,
			Type:  meta.DataType,
			Value: val,
			TS:    ts,
		}
	}

	api.RespondJSON(w, http.StatusOK, 0, "OK", results, ServiceName)
}

type BatchSetRequest struct {
	Values map[string]float64 `json:"values"`
}

func handleBatchSet(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	var req BatchSetRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid request payload", ServiceName)
		return
	}

	mockLock.RLock()
	isMocked := mockEnabled
	mockLock.RUnlock()

	results := make(map[string]string)
	now := uint64(time.Now().UnixNano() / 1e6)

	for idStr, val := range req.Values {
		id, err := strconv.ParseUint(idStr, 10, 32)
		if err != nil {
			results[idStr] = "Invalid Tag ID"
			continue
		}

		if isMocked {
			mockLock.Lock()
			mockValues[uint32(id)] = val
			mockLock.Unlock()
			results[idStr] = "OK"
		} else {
			if matrix == nil {
				api.RespondError(w, http.StatusServiceUnavailable, "DataHub not connected", ServiceName)
				return
			}
			err = matrix.WriteTag(uint32(id), val, now)
			if err != nil {
				results[idStr] = err.Error()
			} else {
				results[idStr] = "OK"
			}
		}
	}

	api.RespondJSON(w, http.StatusOK, 0, "Batch write completed", results, ServiceName)
}

type LogLevelRequest struct {
	Level string `json:"level"`
}

func handleLogging(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPut {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	var req LogLevelRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		api.RespondError(w, http.StatusBadRequest, "Invalid request payload", ServiceName)
		return
	}

	level := strings.ToLower(req.Level)
	if level != "debug" && level != "info" && level != "warn" && level != "error" {
		api.RespondError(w, http.StatusBadRequest, "Invalid log level", ServiceName)
		return
	}

	logLevelMutex.Lock()
	currentLogLevel = level
	logLevelMutex.Unlock()

	log.Printf("[SYSTEM] Log level dynamically switched to %s. Capturing HEX frame dumps.", strings.ToUpper(level))

	api.RespondJSON(w, http.StatusOK, 0, "Log level updated successfully", map[string]string{"current_level": level}, ServiceName)
}

func handleBackup(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	dbPath := "/workspaces/edgeos/EdgeC/build/products/iems/bin/iems.db"
	backupPath := "/tmp/edgeos_backup.db"

	var backupType string
	var err error

	if _, err = os.Stat(dbPath); err == nil {
		var input []byte
		input, err = os.ReadFile(dbPath)
		if err == nil {
			err = os.WriteFile(backupPath, input, 0644)
		}
		backupType = "SQLite database copy"
	} else {
		backupPath = "/tmp/edgeos_snapshot.json"
		mockLock.RLock()
		snapshotData, err2 := json.MarshalIndent(mockValues, "", "  ")
		mockLock.RUnlock()

		if err2 == nil {
			err = os.WriteFile(backupPath, snapshotData, 0644)
		} else {
			err = err2
		}
		backupType = "Mock values JSON snapshot"
	}

	if err != nil {
		api.RespondError(w, http.StatusInternalServerError, "Failed to create backup: "+err.Error(), ServiceName)
		return
	}

	log.Printf("[SYSTEM] Backup completed successfully. Type: %s, Destination: %s", backupType, backupPath)

	api.RespondJSON(w, http.StatusOK, 0, "Backup completed successfully", map[string]string{
		"backup_type": backupType,
		"backup_path": backupPath,
	}, ServiceName)
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
	mux.HandleFunc("/api/v1/ui/mock", handleMockToggle)
	mux.HandleFunc("/api/v1/hub/batch/read", handleBatchRead)
	mux.HandleFunc("/api/v1/actor/batch/set", handleBatchSet)
	mux.HandleFunc("/api/v1/system/logging", handleLogging)
	mux.HandleFunc("/api/v1/system/backup", handleBackup)

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
