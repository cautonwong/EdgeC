package main

import (
	"encoding/json"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"sync"

	"edgeos.go/pkg/api"
	"edgeos.go/pkg/service"
)

// A simple in-memory config store for demo purposes.
// In production, this would be backed by SQLite (iems.db).
var (
	configStore = make(map[string]interface{})
	configMutex sync.RWMutex
)

// FileServer interceptor for Vue SPA
type spaFileSystem struct {
	root http.FileSystem
}

func (fs *spaFileSystem) Open(name string) (http.File, error) {
	f, err := fs.root.Open(name)
	if os.IsNotExist(err) {
		return fs.root.Open("index.html")
	}
	return f, err
}

func main() {
	svc := service.New("ui_host", 8082)

	// API: Get Layout Config
	svc.Handle("/api/v1/ui/config", func(w http.ResponseWriter, r *http.Request) {
		if r.Method == http.MethodGet {
			configMutex.RLock()
			defer configMutex.RUnlock()
			api.RespondJSON(w, http.StatusOK, 0, "OK", configStore, svc.Name)
		} else if r.Method == http.MethodPost {
			var newConfig map[string]interface{}
			if err := json.NewDecoder(r.Body).Decode(&newConfig); err != nil {
				api.RespondError(w, http.StatusBadRequest, "Invalid JSON", svc.Name)
				return
			}
			configMutex.Lock()
			configStore = newConfig
			configMutex.Unlock()
			api.RespondJSON(w, http.StatusOK, 0, "Config saved", nil, svc.Name)
		} else {
			api.RespondError(w, http.StatusMethodNotAllowed, "Method not allowed", svc.Name)
		}
	})

	staticDir := os.Getenv("STATIC_DIR")
	if staticDir == "" {
		staticDir = "/workspaces/edgeos/EdgeC/web/dist"
	}
	os.MkdirAll(staticDir, 0755)
	
	// Create a dummy index.html for testing
	dummyHTML := filepath.Join(staticDir, "index.html")
	if _, err := os.Stat(dummyHTML); os.IsNotExist(err) {
		os.WriteFile(dummyHTML, []byte("<h1>EdgeOS Command Center (Placeholder)</h1>"), 0644)
	}

	fs := http.FileServer(&spaFileSystem{http.Dir(staticDir)})
	svc.Handle("/", func(w http.ResponseWriter, r *http.Request) {
		// Log static file requests but serve normally
		fs.ServeHTTP(w, r)
	})

	if err := svc.Run(); err != nil {
		log.Fatalf("UI Host failed: %v", err)
	}
}
