package webrtc

import (
	"testing"
)

func TestWebRTCCallLifecycle(t *testing.T) {
	mgr := NewCallManager()

	// 1. Invalid initiation
	_, err := mgr.InitiateCall("", "")
	if err == nil {
		t.Fatalf("expected error for empty params")
	}

	// 2. Initiate valid call
	offer := "v=0\r\no=- 12345 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"
	session, err := mgr.InitiateCall("doctor_station_01", offer)
	if err != nil {
		t.Fatalf("unexpected error initiating call: %v", err)
	}

	if session.State != StateDialing {
		t.Errorf("expected state %s, got %s", StateDialing, session.State)
	}

	// 3. Add ICE candidate
	cand := IceCandidate{
		Candidate:     "candidate:842163049 1 udp 16777215 192.168.1.100 52341 typ host",
		SDPMid:        "0",
		SDPMLineIndex: 0,
	}
	if err := mgr.AddIceCandidate(session.SessionID, cand); err != nil {
		t.Fatalf("failed to add ICE candidate: %v", err)
	}

	// 4. Accept Call with Answer SDP
	answer := "v=0\r\no=- 54321 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n"
	if err := mgr.AcceptCall(session.SessionID, answer); err != nil {
		t.Fatalf("failed to accept call: %v", err)
	}

	sessCopy, err := mgr.GetSession(session.SessionID)
	if err != nil {
		t.Fatalf("failed to get session: %v", err)
	}

	if sessCopy.State != StateConnected {
		t.Errorf("expected connected state, got %s", sessCopy.State)
	}
	if len(sessCopy.Candidates) != 1 {
		t.Errorf("expected 1 candidate, got %d", len(sessCopy.Candidates))
	}

	// 5. End Call
	if err := mgr.EndCall(session.SessionID); err != nil {
		t.Fatalf("failed to end call: %v", err)
	}

	sessEnd, _ := mgr.GetSession(session.SessionID)
	if sessEnd.State != StateDisconnected {
		t.Errorf("expected disconnected state, got %s", sessEnd.State)
	}
}
