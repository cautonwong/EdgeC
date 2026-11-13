package main

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"math"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"
	"unsafe"

	"edgeos.go/internal/apptask"
	"edgeos.go/internal/platformbackup"
	"edgeos.go/pkg/api"
	"edgeos.go/pkg/shm"
	"edgeos.go/pkg/tags"
)

const ServiceName = "edge_api_core"

var (
	matrix          *shm.ShadowMatrix
	mockLock        sync.RWMutex
	mockEnabled     bool
	mockValues      = make(map[uint32]float64)
	mockWatchers    = make([]chan *shm.PipeMsg, 0)
	watchersLock    sync.Mutex
	mockCancel      context.CancelFunc
	currentLogLevel = "info"
	logLevelMutex   sync.RWMutex
	// maintenanceMode rejects every write while a restore is rewriting the
	// control database. It is set before the copy and cleared only after the
	// restored database and fresh session tokens are in place.
	maintenanceMode atomic.Bool
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

func databasePath() string {
	if path := os.Getenv("EDGE_DB_PATH"); path != "" {
		return path
	}
	return "iems.db"
}

func backupBaseDir() string {
	if dir := os.Getenv("EDGE_BACKUP_DIR"); dir != "" {
		return dir
	}
	return "/var/lib/edgeos/backups"
}

func platformVersion() string {
	if version := os.Getenv("EDGE_PLATFORM_VERSION"); version != "" {
		return version
	}
	return platformbackup.DefaultPlatformVersion
}

func supportedSchemaVersion() int {
	if value := os.Getenv("EDGE_SCHEMA_VERSION"); value != "" {
		if version, err := strconv.Atoi(value); err == nil && version > 0 {
			return version
		}
	}
	return platformbackup.DefaultSchemaVersion
}

func rotateSessionTokens() error {
	path := os.Getenv("EDGE_SESSION_TOKEN_FILE")
	if path == "" {
		return nil
	}
	return platformbackup.RotateTokenFile(path)
}

func handleBackup(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	backupPath := filepath.Join(backupBaseDir(),
		"backup-"+time.Now().UTC().Format("20060102T150405.000000000Z"))

	key := strings.TrimSpace(r.Header.Get("Idempotency-Key"))
	if key == "" {
		var body struct {
			IdempotencyKey string `json:"idempotency_key"`
		}
		_ = json.NewDecoder(r.Body).Decode(&body)
		key = strings.TrimSpace(body.IdempotencyKey)
	}
	if key == "" {
		key = fmt.Sprintf("backup-%d", time.Now().UnixNano())
	}

	task, replayed, err := apptask.Submit(databasePath(), apptask.KindBackup, key, time.Now())
	if err != nil {
		api.RespondError(w, http.StatusInternalServerError,
			"Failed to record backup task: "+err.Error(), ServiceName)
		return
	}
	if replayed {
		switch task.State {
		case apptask.StateSucceeded:
			api.RespondJSON(w, http.StatusOK, 0, "Backup already completed",
				map[string]interface{}{"replayed": true, "task_id": task.ID}, ServiceName)
		case apptask.StateFailed:
			api.RespondError(w, http.StatusConflict,
				"Backup with this idempotency key already failed", ServiceName)
		default:
			api.RespondError(w, http.StatusConflict, "Backup already in progress", ServiceName)
		}
		return
	}

	ctx, cancel := context.WithTimeout(r.Context(), 5*time.Minute)
	defer cancel()

	manifest, err := platformbackup.Create(ctx, platformbackup.CreateOptions{
		DBPath:          databasePath(),
		DestDir:         backupPath,
		PlatformVersion: platformVersion(),
	})
	if err != nil {
		_ = apptask.Finish(databasePath(), task.ID, apptask.StateFailed, err.Error(), time.Now())
		api.RespondError(w, http.StatusInternalServerError, "Failed to create backup: "+err.Error(), ServiceName)
		return
	}

	result, _ := json.Marshal(map[string]string{"backup_path": backupPath})
	_ = apptask.Finish(databasePath(), task.ID, apptask.StateSucceeded, string(result), time.Now())

	log.Printf("[SYSTEM] Backup completed: %s (schema %d, %d file(s))",
		backupPath, manifest.SchemaVersion, len(manifest.Files))

	api.RespondJSON(w, http.StatusOK, 0, "Backup completed successfully", map[string]interface{}{
		"backup_type": "sqlite_online_backup",
		"backup_path": backupPath,
		"task_id":     task.ID,
		"manifest":    manifest,
	}, ServiceName)
}

func handleRestore(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", ServiceName)
		return
	}

	var req struct {
		BackupPath string `json:"backup_path"`
	}
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil || req.BackupPath == "" {
		api.RespondError(w, http.StatusBadRequest, "Invalid request payload", ServiceName)
		return
	}
	if !maintenanceMode.CompareAndSwap(false, true) {
		api.RespondError(w, http.StatusLocked, "Restore already in progress", ServiceName)
		return
	}
	keepMaintenance := false
	defer func() {
		if !keepMaintenance {
			maintenanceMode.Store(false)
		}
	}()

	ctx, cancel := context.WithTimeout(r.Context(), 5*time.Minute)
	defer cancel()

	preRestoreDir := filepath.Join(backupBaseDir(),
		"pre-restore-"+time.Now().UTC().Format("20060102T150405.000000000Z"))
	result, err := platformbackup.Restore(ctx, platformbackup.RestoreOptions{
		BackupDir:              req.BackupPath,
		DBPath:                 databasePath(),
		PlatformVersion:        platformVersion(),
		SupportedSchemaVersion: supportedSchemaVersion(),
		PreRestoreBackupDir:    preRestoreDir,
		RotateTokens:           rotateSessionTokens,
	})
	if err != nil {
		// A failed copy rolls back; keep maintenance only when the database
		// was restored but old session tokens could not be replaced.
		keepMaintenance = errors.Is(err, platformbackup.ErrTokenRotation)
		status := http.StatusInternalServerError
		switch {
		case errors.Is(err, platformbackup.ErrBackupTooNew):
			status = http.StatusConflict
		case errors.Is(err, platformbackup.ErrInvalidManifest),
			errors.Is(err, platformbackup.ErrUnsupportedFormat),
			errors.Is(err, platformbackup.ErrHashMismatch):
			status = http.StatusBadRequest
		}
		api.RespondError(w, status, "Restore failed: "+err.Error(), ServiceName)
		return
	}

	log.Printf("[SYSTEM] Restore completed from %s; session tokens rotated: %v",
		req.BackupPath, result.SessionTokensRotated)
	api.RespondJSON(w, http.StatusOK, 0, "Restore completed successfully", result, ServiceName)
}

// maintenanceGuard returns 423 for writes while a restore is in progress.
// Reads stay available so the UI can show maintenance state.
func maintenanceGuard(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.Method {
		case http.MethodGet, http.MethodHead, http.MethodOptions:
		default:
			if maintenanceMode.Load() {
				api.RespondError(w, http.StatusLocked,
					"Maintenance mode: write operations are disabled during restore", ServiceName)
				return
			}
		}
		next.ServeHTTP(w, r)
	})
}

// 💡 [ADR-0001] Prometheus-format metrics endpoint
func handleMetrics(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/plain; version=0.0.4")
	w.WriteHeader(http.StatusOK)

	totalTags := len(tags.Metadata)
	mockLock.RLock()
	mockCount := len(mockValues)
	mockLock.RUnlock()

	fmt.Fprintf(w, "# HELP edgeos_tags_total Total number of registered tags\n")
	fmt.Fprintf(w, "# TYPE edgeos_tags_total gauge\n")
	fmt.Fprintf(w, "edgeos_tags_total %d\n", totalTags)

	fmt.Fprintf(w, "# HELP edgeos_mock_enabled Whether mock mode is active\n")
	fmt.Fprintf(w, "# TYPE edgeos_mock_enabled gauge\n")
	mockVal := 0
	if mockEnabled {
		mockVal = 1
	}
	fmt.Fprintf(w, "edgeos_mock_enabled %d\n", mockVal)

	fmt.Fprintf(w, "# HELP edgeos_mock_values_count Number of mock values being generated\n")
	fmt.Fprintf(w, "# TYPE edgeos_mock_values_count gauge\n")
	fmt.Fprintf(w, "edgeos_mock_values_count %d\n", mockCount)

	shmConnected := 0
	if matrix != nil {
		shmConnected = 1
	}
	fmt.Fprintf(w, "# HELP edgeos_shm_connected SHM connection status\n")
	fmt.Fprintf(w, "# TYPE edgeos_shm_connected gauge\n")
	fmt.Fprintf(w, "edgeos_shm_connected %d\n", shmConnected)

	fmt.Fprintf(w, "# HELP edgeos_build_info Build metadata\n")
	fmt.Fprintf(w, "# TYPE edgeos_build_info gauge\n")
	fmt.Fprintf(w, "edgeos_build_info{service=\"%s\"} 1\n", ServiceName)
}

func main() {

	log.Printf("Starting %s on port 8080...", ServiceName)

	shmName := os.Getenv("EDGE_SHM_NAME")
	if shmName == "" {
		shmName = "iems"
	}
	var err error
	matrix, err = shm.OpenShadowMatrix(shmName, 1024)
	if err != nil {
		log.Printf("Warning: Failed to open DataHub SHM: %v", err)
	} else {
		log.Printf("Successfully connected to DataHub SHM: edgeos_shadow_%s", shmName)
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
	mux.HandleFunc("/api/v1/system/restore", handleRestore)
	mux.HandleFunc("/api/v1/system/metrics", handleMetrics)

	// Wrap with middlewares
	recovery := func(next http.Handler) http.Handler {
		return api.RecoveryMiddleware(next, ServiceName)
	}
	handler := api.Chain(mux, api.LoggerMiddleware, recovery, maintenanceGuard)

	server := &http.Server{
		Addr:    ":8080",
		Handler: handler,
	}

	log.Fatal(server.ListenAndServe())
}
