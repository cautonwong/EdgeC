package service

import (
	"log"
	"net/http"
	"strconv"
	"strings"

	"edgeos.go/pkg/api"
)

// EdgeService is the standard skeleton for EdgeOS microservices
type EdgeService struct {
	Name    string
	Port    int
	mux     *http.ServeMux
	authKey string
}

// New creates a new EdgeService instance
func New(name string, port int) *EdgeService {
	s := &EdgeService{
		Name: name,
		Port: port,
		mux:  http.NewServeMux(),
	}

	// Default health endpoint
	s.mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		api.RespondJSON(w, http.StatusOK, 0, "OK", map[string]string{"status": "running"}, s.Name)
	})

	return s
}

// SetAPIKey secures specific endpoints with X-API-KEY header
func (s *EdgeService) SetAPIKey(key string) {
	s.authKey = key
}

// Handle registers a new endpoint route
func (s *EdgeService) Handle(pattern string, handler func(http.ResponseWriter, *http.Request)) {
	s.mux.HandleFunc(pattern, handler)
}

// authMiddleware enforces X-API-KEY check for sensitive routes (e.g. POST)
func (s *EdgeService) authMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if s.authKey != "" && (r.Method == http.MethodPost || r.Method == http.MethodPut || r.Method == http.MethodDelete) {
			key := r.Header.Get("X-API-KEY")
			if key != s.authKey {
				api.RespondError(w, http.StatusUnauthorized, "Unauthorized", s.Name)
				return
			}
		}
		next.ServeHTTP(w, r)
	})
}

// Run starts the microservice
func (s *EdgeService) Run() error {
	recovery := func(next http.Handler) http.Handler {
		return api.RecoveryMiddleware(next, s.Name)
	}

	// Chain middlewares
	handler := api.Chain(s.mux, api.LoggerMiddleware, recovery, s.authMiddleware)

	server := &http.Server{
		Addr:    ":" + strconv.Itoa(s.Port),
		Handler: handler,
	}

	log.Printf("[%s] Starting on port %d...", strings.ToUpper(s.Name), s.Port)
	return server.ListenAndServe()
}
