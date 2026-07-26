package prescription

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"sync"
	"testing"
	"time"
)

func TestPrescriptionValidation(t *testing.T) {
	mgr := NewManager()

	var warningReceived string
	var warningMu sync.Mutex
	mgr.OnWarning = func(w string) {
		warningMu.Lock()
		warningReceived = w
		warningMu.Unlock()
	}

	// 1. Test validation with no active prescription
	resNoActive := mgr.ValidateTrainingItem("ITEM_GAIT_01", "RX-2026-001")
	if resNoActive.Valid {
		t.Errorf("Expected valid=false when no active prescription set")
	}
	if !strings.Contains(resNoActive.Warning, "No active rehabilitation prescription") {
		t.Errorf("Unexpected warning message: %s", resNoActive.Warning)
	}

	warningMu.Lock()
	if !strings.Contains(warningReceived, "No active rehabilitation prescription") {
		t.Errorf("Expected warning callback to receive message, got: %s", warningReceived)
	}
	warningMu.Unlock()

	// Set active prescription
	rx := &Prescription{
		ID:          "RX-2026-001",
		PatientID:   "PATIENT-881",
		DoctorID:    "DOC-009",
		TargetItems: []string{"ITEM_GAIT_01", "ITEM_BALANCE_02"},
		Intensity:   "Moderate",
		DurationMin: 30,
	}
	mgr.SetPrescription(rx)

	active := mgr.GetActivePrescription()
	if active == nil || active.ID != "RX-2026-001" {
		t.Fatalf("Failed to retrieve active prescription")
	}

	// 2. Test valid prescription item
	resValid := mgr.ValidateTrainingItem("ITEM_GAIT_01", "RX-2026-001")
	if !resValid.Valid || resValid.Warning != "" {
		t.Errorf("Expected valid=true for correct prescription item, got valid=%v, warning=%s", resValid.Valid, resValid.Warning)
	}

	// 3. Test prescription ID mismatch (REHAB_PRESCRIPTION_ID mismatch)
	resIDMismatch := mgr.ValidateTrainingItem("ITEM_GAIT_01", "RX-MISMATCH-999")
	if resIDMismatch.Valid {
		t.Errorf("Expected valid=false for prescription ID mismatch")
	}
	if !strings.Contains(resIDMismatch.Warning, "mismatch with active prescription ID") {
		t.Errorf("Unexpected warning for ID mismatch: %s", resIDMismatch.Warning)
	}

	// 4. Test unprescribed item ID
	resItemMismatch := mgr.ValidateTrainingItem("ITEM_UNAUTHORIZED_99", "RX-2026-001")
	if resItemMismatch.Valid {
		t.Errorf("Expected valid=false for unprescribed item")
	}
	if !strings.Contains(resItemMismatch.Warning, "not prescribed in active prescription") {
		t.Errorf("Unexpected warning for item mismatch: %s", resItemMismatch.Warning)
	}
}

func TestAdherenceTracking(t *testing.T) {
	mgr := NewManager()

	var warningMsg string
	var wg sync.WaitGroup
	wg.Add(1)
	mgr.OnWarning = func(w string) {
		warningMsg = w
		wg.Done()
	}

	mgr.RecordSession("ITEM_GAIT_01", "RX-2026-001", true)
	mgr.RecordSession("ITEM_GAIT_01", "RX-2026-001", true)
	_, warn := mgr.RecordSession("ITEM_GAIT_01", "RX-2026-001", true)

	if warn == "" {
		t.Errorf("Expected adherence warning after 3 interruptions")
	}

	wg.Wait()
	if !strings.Contains(warningMsg, "adherence drop detected") {
		t.Errorf("Unexpected warning message: %s", warningMsg)
	}
}

func TestWebSocketPrescriptionSync(t *testing.T) {
	mgr := NewManager()

	server := httptest.NewServer(http.HandlerFunc(mgr.HandleWebSocket))
	defer server.Close()

	wsURL := "ws" + strings.TrimPrefix(server.URL, "http")

	conn, err := mgr.ConnectCloudWebSocket(wsURL)
	if err != nil {
		t.Fatalf("Failed to connect WebSocket client: %v", err)
	}
	defer conn.Close()

	// Send sync_prescription from client to server over WS
	newRx := &Prescription{
		ID:          "RX-CLOUD-777",
		PatientID:   "PATIENT-999",
		DoctorID:    "DOC-101",
		TargetItems: []string{"ITEM_GAIT_01"},
		Intensity:   "High",
		DurationMin: 45,
	}

	msgSync := WSMessage{
		Action:       "sync_prescription",
		Prescription: newRx,
		Timestamp:    time.Now().Unix(),
	}

	payload, _ := json.Marshal(msgSync)
	if err := mgr.writeFrame(conn, 0x1, payload); err != nil {
		t.Fatalf("Failed to send WS message: %v", err)
	}

	time.Sleep(50 * time.Millisecond)

	active := mgr.GetActivePrescription()
	if active == nil || active.ID != "RX-CLOUD-777" {
		t.Fatalf("Expected active prescription ID 'RX-CLOUD-777', got %v", active)
	}

	// Send validate_training request over WS
	msgVal := WSMessage{
		Action:         "validate_training",
		ItemID:         "ITEM_GAIT_01",
		PrescriptionID: "RX-CLOUD-777",
		Timestamp:      time.Now().Unix(),
	}

	payloadVal, _ := json.Marshal(msgVal)
	if err := mgr.writeFrame(conn, 0x1, payloadVal); err != nil {
		t.Fatalf("Failed to write validate_training frame: %v", err)
	}

	time.Sleep(50 * time.Millisecond)
}
