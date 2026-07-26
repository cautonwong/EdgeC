package webrtc

import (
	"crypto/rand"
	"encoding/hex"
	"errors"
	"sync"
	"time"
)

// CallState represents the lifecycle state of a P2P WebRTC video call.
type CallState string

const (
	StateIdle         CallState = "IDLE"
	StateDialing      CallState = "DIALING"
	StateRinging      CallState = "RINGING"
	StateConnected    CallState = "CONNECTED"
	StateDisconnected CallState = "DISCONNECTED"
	StateFailed       CallState = "FAILED"
)

// SessionDescription holds SDP offer or answer data.
type SessionDescription struct {
	Type string `json:"type"` // "offer" or "answer"
	SDP  string `json:"sdp"`
}

// IceCandidate represents a network candidate for NAT traversal.
type IceCandidate struct {
	Candidate     string `json:"candidate"`
	SDPMid        string `json:"sdpMid"`
	SDPMLineIndex int    `json:"sdpMLineIndex"`
}

// CallSession tracks a single WebRTC remote care session.
type CallSession struct {
	SessionID   string
	PeerID      string
	State       CallState
	LocalSDP    *SessionDescription
	RemoteSDP   *SessionDescription
	Candidates  []IceCandidate
	StartTime   time.Time
	ConnectedAt time.Time
	EndedAt     time.Time
}

// CallManager handles WebRTC P2P signaling and session tracking for remote care.
type CallManager struct {
	mu       sync.RWMutex
	sessions map[string]*CallSession
}

// NewCallManager creates a new WebRTC CallManager instance.
func NewCallManager() *CallManager {
	return &CallManager{
		sessions: make(map[string]*CallSession),
	}
}

func generateID() string {
	b := make([]byte, 8)
	_, _ = rand.Read(b)
	return hex.EncodeToString(b)
}

// InitiateCall starts a new outgoing video call to a target peer.
func (m *CallManager) InitiateCall(peerID string, offerSDP string) (*CallSession, error) {
	if peerID == "" || offerSDP == "" {
		return nil, errors.New("invalid peerID or offer SDP")
	}

	m.mu.Lock()
	defer m.mu.Unlock()

	session := &CallSession{
		SessionID: generateID(),
		PeerID:    peerID,
		State:     StateDialing,
		LocalSDP: &SessionDescription{
			Type: "offer",
			SDP:  offerSDP,
		},
		Candidates: make([]IceCandidate, 0),
		StartTime:  time.Now(),
	}

	m.sessions[session.SessionID] = session
	return session, nil
}

// AcceptCall accepts an incoming SDP answer for a dialing/ringing call session.
func (m *CallManager) AcceptCall(sessionID string, answerSDP string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	session, exists := m.sessions[sessionID]
	if !exists {
		return errors.New("session not found")
	}

	if session.State != StateDialing && session.State != StateRinging {
		return errors.New("invalid call state for accept")
	}

	session.RemoteSDP = &SessionDescription{
		Type: "answer",
		SDP:  answerSDP,
	}
	session.State = StateConnected
	session.ConnectedAt = time.Now()

	return nil
}

// AddIceCandidate adds an ICE candidate to an active call session.
func (m *CallManager) AddIceCandidate(sessionID string, candidate IceCandidate) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	session, exists := m.sessions[sessionID]
	if !exists {
		return errors.New("session not found")
	}

	session.Candidates = append(session.Candidates, candidate)
	return nil
}

// EndCall terminates an active or dialing call session.
func (m *CallManager) EndCall(sessionID string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	session, exists := m.sessions[sessionID]
	if !exists {
		return errors.New("session not found")
	}

	session.State = StateDisconnected
	session.EndedAt = time.Now()
	return nil
}

// GetSession returns a copy of the specified session detail.
func (m *CallManager) GetSession(sessionID string) (*CallSession, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	session, exists := m.sessions[sessionID]
	if !exists {
		return nil, errors.New("session not found")
	}

	cp := *session
	return &cp, nil
}
