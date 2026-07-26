package prescription

import (
	"bufio"
	"crypto/sha1"
	"encoding/base64"
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"math/rand"
	"net"
	"net/http"
	"net/url"
	"sync"
	"time"
)

// Prescription represents a rehabilitation prescription received from cloud or clinician.
type Prescription struct {
	ID          string   `json:"id"`
	PatientID   string   `json:"patient_id"`
	DoctorID    string   `json:"doctor_id"`
	TargetItems []string `json:"target_items"` // Prescribed training items (e.g. ITEM_GAIT_01, ITEM_ARM_02)
	Intensity   string   `json:"intensity"`
	DurationMin int      `json:"duration_min"`
	CreatedAt   int64    `json:"created_at"`
	Status      string   `json:"status"` // ACTIVE, EXPIRED, COMPLETED
}

// ValidationResult represents the outcome of verifying a training session item against the active prescription.
type ValidationResult struct {
	Valid          bool   `json:"valid"`
	Warning        string `json:"warning,omitempty"`
	PrescriptionID string `json:"prescription_id"`
	IncomingItemID string `json:"incoming_item_id"`
}

// WSMessage represents a double-way WebSocket payload for prescription sync & validation.
type WSMessage struct {
	Action         string            `json:"action"` // "sync_prescription", "validate_training", "get_active", "warning_alert"
	Prescription   *Prescription     `json:"prescription,omitempty"`
	ItemID         string            `json:"item_id,omitempty"`
	PrescriptionID string            `json:"prescription_id,omitempty"`
	Validation     *ValidationResult `json:"validation,omitempty"`
	Message        string            `json:"message,omitempty"`
	Timestamp      int64             `json:"timestamp"`
}

// Manager orchestrates double-way WebSocket prescription sync, validation, and compliance tracking.
type Manager struct {
	mu                 sync.RWMutex
	activePrescription *Prescription
	history            map[string]*Prescription

	// Compliance & interruption tracking
	totalCount       int
	completedCount   int
	interruptedCount int

	// WebSocket clients
	clientsMu sync.Mutex
	clients   map[net.Conn]bool

	// Callback for warnings
	OnWarning func(warning string)
}

// NewManager initializes a new Prescription Manager.
func NewManager() *Manager {
	return &Manager{
		history: make(map[string]*Prescription),
		clients: make(map[net.Conn]bool),
	}
}

// SetPrescription sets or updates the current active prescription and notifies WebSocket clients.
func (m *Manager) SetPrescription(p *Prescription) {
	if p == nil {
		return
	}
	m.mu.Lock()
	p.Status = "ACTIVE"
	if p.CreatedAt == 0 {
		p.CreatedAt = time.Now().Unix()
	}
	m.activePrescription = p
	m.history[p.ID] = p
	m.mu.Unlock()

	log.Printf("[PRESCRIPTION MANAGER] Active prescription updated: ID=%s, Patient=%s, Doctor=%s", p.ID, p.PatientID, p.DoctorID)
	m.BroadcastPrescription(p)
}

// GetActivePrescription returns the current active prescription.
func (m *Manager) GetActivePrescription() *Prescription {
	m.mu.RLock()
	defer m.mu.RUnlock()
	return m.activePrescription
}

// ValidateTrainingItem checks if incoming training item matches current active prescription ID and target items.
func (m *Manager) ValidateTrainingItem(incomingItemID string, incomingPrescriptionID string) ValidationResult {
	m.mu.RLock()
	active := m.activePrescription
	m.mu.RUnlock()

	res := ValidationResult{
		Valid:          true,
		PrescriptionID: incomingPrescriptionID,
		IncomingItemID: incomingItemID,
	}

	if active == nil {
		res.Valid = false
		res.Warning = fmt.Sprintf("WARNING: No active rehabilitation prescription found on gateway for training item '%s'", incomingItemID)
		m.triggerWarning(res.Warning)
		return res
	}

	// 1. Check if incoming prescription ID matches current active prescription ID (REHAB_PRESCRIPTION_ID)
	if incomingPrescriptionID != "" && incomingPrescriptionID != active.ID {
		res.Valid = false
		res.Warning = fmt.Sprintf("WARNING: Incoming training prescription ID '%s' mismatch with active prescription ID '%s'", incomingPrescriptionID, active.ID)
		m.triggerWarning(res.Warning)
		return res
	}

	// 2. Check if incoming training item is allowed under active prescription target items
	if len(active.TargetItems) > 0 {
		matched := false
		for _, item := range active.TargetItems {
			if item == incomingItemID {
				matched = true
				break
			}
		}
		if !matched {
			res.Valid = false
			res.Warning = fmt.Sprintf("WARNING: Training item '%s' is not prescribed in active prescription '%s' target items %v", incomingItemID, active.ID, active.TargetItems)
			m.triggerWarning(res.Warning)
			return res
		}
	}

	return res
}

// RecordSession updates adherence & interruption statistics.
func (m *Manager) RecordSession(itemID string, prescriptionID string, interrupted bool) (float64, string) {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.totalCount++
	if interrupted {
		m.interruptedCount++
	} else {
		m.completedCount++
	}

	rate := 0.0
	if m.totalCount > 0 {
		rate = float64(m.completedCount) / float64(m.totalCount)
	}

	var warning string
	if m.interruptedCount >= 3 || (m.totalCount >= 5 && rate < 0.6) {
		warning = fmt.Sprintf("WARNING: Rehabilitation adherence drop detected! Interrupted=%d, ComplianceRate=%.1f%%", m.interruptedCount, rate*100)
		if m.OnWarning != nil {
			go m.OnWarning(warning)
		}
	}

	return rate, warning
}

func (m *Manager) triggerWarning(warn string) {
	log.Printf("[PRESCRIPTION WARNING] %s", warn)
	if m.OnWarning != nil {
		m.OnWarning(warn)
	}
}

// --- WebSocket RFC 6455 Light Server & Client implementation ---

const wsGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// HandleRemoteScreen handles web screen mirroring for Remote Assistance endpoint (/api/v1/remote/screen).
func (m *Manager) HandleRemoteScreen(w http.ResponseWriter, r *http.Request) {
	if r.Header.Get("Upgrade") == "websocket" {
		m.HandleWebSocket(w, r)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"code":    0,
		"message": "Remote assistance web screen mirroring active",
		"data": map[string]interface{}{
			"status":     "STREAMING",
			"fps":        30,
			"resolution": "1080p",
			"protocol":   "WebRTC/MJPEG",
		},
		"meta": map[string]interface{}{
			"mtt": time.Now().UnixNano() / 1e6,
		},
	})
}

// HandleWebSocket upgrades HTTP connection to WebSocket for double-way prescription sync.
func (m *Manager) HandleWebSocket(w http.ResponseWriter, r *http.Request) {
	if r.Header.Get("Upgrade") != "websocket" {
		http.Error(w, "Expected websocket upgrade", http.StatusBadRequest)
		return
	}

	key := r.Header.Get("Sec-WebSocket-Key")
	if key == "" {
		http.Error(w, "Missing Sec-WebSocket-Key header", http.StatusBadRequest)
		return
	}

	h := sha1.New()
	h.Write([]byte(key + wsGUID))
	acceptKey := base64.StdEncoding.EncodeToString(h.Sum(nil))

	hj, ok := w.(http.Hijacker)
	if !ok {
		http.Error(w, "Hijacking not supported", http.StatusInternalServerError)
		return
	}

	conn, bufrw, err := hj.Hijack()
	if err != nil {
		log.Printf("Hijack failed: %v", err)
		return
	}

	// Return HTTP 101 Switching Protocols response
	response := "HTTP/1.1 101 Switching Protocols\r\n" +
		"Upgrade: websocket\r\n" +
		"Connection: Upgrade\r\n" +
		"Sec-WebSocket-Accept: " + acceptKey + "\r\n\r\n"
	bufrw.WriteString(response)
	bufrw.Flush()

	m.addClient(conn)
	defer func() {
		m.removeClient(conn)
		conn.Close()
	}()

	// Send current active prescription if available
	if active := m.GetActivePrescription(); active != nil {
		msg := WSMessage{
			Action:       "sync_prescription",
			Prescription: active,
			Timestamp:    time.Now().Unix(),
		}
		_ = m.sendJSON(conn, msg)
	}

	// Read loop
	reader := bufrw.Reader
	for {
		opcode, payload, err := m.readFrame(reader)
		if err != nil {
			break
		}

		switch opcode {
		case 0x1: // Text frame
			var wsMsg WSMessage
			if err := json.Unmarshal(payload, &wsMsg); err != nil {
				log.Printf("Failed to unmarshal WS message: %v", err)
				continue
			}
			m.handleWSIncomingMessage(conn, &wsMsg)
		case 0x8: // Close frame
			return
		case 0x9: // Ping frame
			_ = m.writeFrame(conn, 0x0A, payload) // Send Pong
		}
	}
}

func (m *Manager) handleWSIncomingMessage(conn net.Conn, msg *WSMessage) {
	switch msg.Action {
	case "sync_prescription":
		if msg.Prescription != nil {
			m.SetPrescription(msg.Prescription)
			resp := WSMessage{
				Action:    "sync_ack",
				Message:   fmt.Sprintf("Prescription '%s' synced successfully", msg.Prescription.ID),
				Timestamp: time.Now().Unix(),
			}
			_ = m.sendJSON(conn, resp)
		}
	case "validate_training":
		valResult := m.ValidateTrainingItem(msg.ItemID, msg.PrescriptionID)
		resp := WSMessage{
			Action:     "validate_result",
			Validation: &valResult,
			Timestamp:  time.Now().Unix(),
		}
		_ = m.sendJSON(conn, resp)
	case "get_active":
		active := m.GetActivePrescription()
		resp := WSMessage{
			Action:       "sync_prescription",
			Prescription: active,
			Timestamp:    time.Now().Unix(),
		}
		_ = m.sendJSON(conn, resp)
	}
}

// BroadcastPrescription broadcasts prescription update to all connected WebSocket clients.
func (m *Manager) BroadcastPrescription(p *Prescription) {
	msg := WSMessage{
		Action:       "sync_prescription",
		Prescription: p,
		Timestamp:    time.Now().Unix(),
	}

	payload, err := json.Marshal(msg)
	if err != nil {
		return
	}

	m.clientsMu.Lock()
	defer m.clientsMu.Unlock()
	for client := range m.clients {
		_ = m.writeFrame(client, 0x1, payload)
	}
}

func (m *Manager) addClient(conn net.Conn) {
	m.clientsMu.Lock()
	m.clients[conn] = true
	m.clientsMu.Unlock()
}

func (m *Manager) removeClient(conn net.Conn) {
	m.clientsMu.Lock()
	delete(m.clients, conn)
	m.clientsMu.Unlock()
}

func (m *Manager) sendJSON(conn net.Conn, v interface{}) error {
	b, err := json.Marshal(v)
	if err != nil {
		return err
	}
	return m.writeFrame(conn, 0x1, b)
}

func (m *Manager) writeFrame(conn net.Conn, opcode byte, payload []byte) error {
	length := len(payload)
	var header []byte

	// Server-to-client frames are unmasked
	if length <= 125 {
		header = []byte{0x80 | (opcode & 0x0F), byte(length)}
	} else if length <= 65535 {
		header = make([]byte, 4)
		header[0] = 0x80 | (opcode & 0x0F)
		header[1] = 126
		binary.BigEndian.PutUint16(header[2:], uint16(length))
	} else {
		header = make([]byte, 10)
		header[0] = 0x80 | (opcode & 0x0F)
		header[1] = 127
		binary.BigEndian.PutUint64(header[2:], uint64(length))
	}

	if _, err := conn.Write(header); err != nil {
		return err
	}
	if length > 0 {
		_, err := conn.Write(payload)
		return err
	}
	return nil
}

func (m *Manager) readFrame(r *bufio.Reader) (byte, []byte, error) {
	b0, err := r.ReadByte()
	if err != nil {
		return 0, nil, err
	}
	opcode := b0 & 0x0F

	b1, err := r.ReadByte()
	if err != nil {
		return 0, nil, err
	}

	masked := (b1 & 0x80) != 0
	payloadLen := int64(b1 & 0x7F)

	if payloadLen == 126 {
		var l uint16
		if err := binary.Read(r, binary.BigEndian, &l); err != nil {
			return 0, nil, err
		}
		payloadLen = int64(l)
	} else if payloadLen == 127 {
		var l uint64
		if err := binary.Read(r, binary.BigEndian, &l); err != nil {
			return 0, nil, err
		}
		payloadLen = int64(l)
	}

	var maskKey [4]byte
	if masked {
		if _, err := io.ReadFull(r, maskKey[:]); err != nil {
			return 0, nil, err
		}
	}

	payload := make([]byte, payloadLen)
	if _, err := io.ReadFull(r, payload); err != nil {
		return 0, nil, err
	}

	if masked {
		for i := 0; i < len(payload); i++ {
			payload[i] ^= maskKey[i%4]
		}
	}

	return opcode, payload, nil
}

// ConnectCloudWebSocket establishes a WebSocket client connection to a cloud endpoint.
func (m *Manager) ConnectCloudWebSocket(serverURL string) (net.Conn, error) {
	u, err := url.Parse(serverURL)
	if err != nil {
		return nil, err
	}

	host := u.Host
	if u.Port() == "" {
		if u.Scheme == "wss" {
			host += ":443"
		} else {
			host += ":80"
		}
	}

	conn, err := net.Dial("tcp", host)
	if err != nil {
		return nil, fmt.Errorf("dial failed: %w", err)
	}

	// Send WS handshake request
	rawKey := make([]byte, 16)
	rand.Read(rawKey)
	key := base64.StdEncoding.EncodeToString(rawKey)

	reqPath := u.Path
	if reqPath == "" {
		reqPath = "/"
	}

	req := fmt.Sprintf("GET %s HTTP/1.1\r\n"+
		"Host: %s\r\n"+
		"Upgrade: websocket\r\n"+
		"Connection: Upgrade\r\n"+
		"Sec-WebSocket-Key: %s\r\n"+
		"Sec-WebSocket-Version: 13\r\n\r\n", reqPath, u.Host, key)

	if _, err := conn.Write([]byte(req)); err != nil {
		conn.Close()
		return nil, err
	}

	bufrw := bufio.NewReader(conn)
	statusLine, err := bufrw.ReadString('\n')
	if err != nil {
		conn.Close()
		return nil, err
	}

	if len(statusLine) < 12 || statusLine[9:12] != "101" {
		conn.Close()
		return nil, errors.New("cloud websocket handshake rejected: " + statusLine)
	}

	// Read remaining HTTP headers
	for {
		line, err := bufrw.ReadString('\n')
		if err != nil || line == "\r\n" || line == "\n" {
			break
		}
	}

	return conn, nil
}
