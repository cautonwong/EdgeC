package api

import (
	"encoding/json"
	"net/http"
	"time"
)

// Meta contains tracing and timestamp info
type Meta struct {
	RequestID string `json:"request_id,omitempty"`
	Timestamp int64  `json:"timestamp"`
	Source    string `json:"source"`
}

// Envelope is the unified response format for EdgeOS APIs
type Envelope struct {
	Code    int         `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data"`
	Meta    Meta        `json:"meta"`
}

// RespondJSON sends a unified JSON envelope
func RespondJSON(w http.ResponseWriter, status int, code int, message string, data interface{}, source string) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)

	env := Envelope{
		Code:    code,
		Message: message,
		Data:    data,
		Meta: Meta{
			Timestamp: time.Now().Unix(),
			Source:    source,
		},
	}

	json.NewEncoder(w).Encode(env)
}

// RespondError is a helper for returning errors
func RespondError(w http.ResponseWriter, status int, message string, source string) {
	RespondJSON(w, status, status, message, nil, source)
}
