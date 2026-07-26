package main

import (
	"bytes"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"database/sql"
	"encoding/json"
	"net"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"edgeos.go/internal/prescription"
	_ "github.com/mattn/go-sqlite3"
)

func TestPurgeAndAnchor(t *testing.T) {
	db, err := sql.Open("sqlite3", ":memory:")
	if err != nil {
		t.Fatal(err)
	}
	defer db.Close()

	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS sensitive_records (timestamp INTEGER, tag_id INTEGER, iv TEXT, ciphertext TEXT, auth_tag TEXT);")
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS tag_history (timestamp INTEGER, tag_id INTEGER, value REAL, status INTEGER);")
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS blockchain_receipts (timestamp INTEGER PRIMARY KEY, tx_hash TEXT, prev_hash TEXT, record_hash TEXT, signature TEXT);")

	priv, _ := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	gateway := &AppGateway{
		db:              db,
		aesKey:          make([]byte, 32),
		jwtSecret:       make([]byte, 32),
		privKey:         priv,
		prescriptionMgr: prescription.NewManager(),
	}
	rand.Read(gateway.aesKey)
	rand.Read(gateway.jwtSecret)

	// 1. Test Login and Token Generation
	adminToken, err := gateway.generateToken("admin_user", RoleAdmin)
	if err != nil {
		t.Fatalf("Failed to generate admin token: %v", err)
	}

	userToken, err := gateway.generateToken("common_user", RoleUser)
	if err != nil {
		t.Fatalf("Failed to generate user token: %v", err)
	}

	// 2. Test saveSensitiveData and decryption
	origVal := "test-heart-rate-99-bpm"
	err = gateway.saveSensitiveData(12005, origVal)
	if err != nil {
		t.Fatalf("Failed to save sensitive data: %v", err)
	}

	var iv, ciphertext, authTag string
	err = db.QueryRow("SELECT iv, ciphertext, auth_tag FROM sensitive_records WHERE tag_id = 12005").Scan(&iv, &ciphertext, &authTag)
	if err != nil {
		t.Fatalf("Failed to query encrypted record: %v", err)
	}

	decrypted, err := gateway.decrypt(iv, ciphertext, authTag)
	if err != nil {
		t.Fatalf("Failed to decrypt data: %v", err)
	}

	if decrypted != origVal {
		t.Errorf("Decryption mismatch: expected %q, got %q", origVal, decrypted)
	}

	// 3. Test RBAC Protection - Unauthenticated Request (expect 401)
	reqPurgeNoAuth := httptest.NewRequest("POST", "/api/v1/privacy/purge", nil)
	wPurgeNoAuth := httptest.NewRecorder()
	gateway.withRBAC(RoleAdmin, gateway.handlePurge)(wPurgeNoAuth, reqPurgeNoAuth)
	if wPurgeNoAuth.Code != http.StatusUnauthorized {
		t.Errorf("Expected 401 Unauthorized for missing token, got %d", wPurgeNoAuth.Code)
	}

	// 4. Test RBAC Protection - Insufficient Role (expect 403 Forbidden)
	reqPurgeUserAuth := httptest.NewRequest("POST", "/api/v1/privacy/purge", nil)
	reqPurgeUserAuth.Header.Set("Authorization", "Bearer "+userToken)
	wPurgeUserAuth := httptest.NewRecorder()
	gateway.withRBAC(RoleAdmin, gateway.handlePurge)(wPurgeUserAuth, reqPurgeUserAuth)
	if wPurgeUserAuth.Code != http.StatusForbidden {
		t.Errorf("Expected 403 Forbidden for insufficient role, got %d", wPurgeUserAuth.Code)
	}

	// 5. Test RBAC Protection - Valid Admin Authorization (expect 200 OK)
	reqPurgeAdminAuth := httptest.NewRequest("POST", "/api/v1/privacy/purge", nil)
	reqPurgeAdminAuth.Header.Set("Authorization", "Bearer "+adminToken)
	wPurgeAdminAuth := httptest.NewRecorder()
	gateway.withRBAC(RoleAdmin, gateway.handlePurge)(wPurgeAdminAuth, reqPurgeAdminAuth)
	if wPurgeAdminAuth.Code != http.StatusOK {
		t.Errorf("Expected 200 OK for valid admin token, got %d", wPurgeAdminAuth.Code)
	}

	// 6. Test Anchor REST Endpoint with Doctor Role Token
	doctorToken, _ := gateway.generateToken("dr_smith", RoleDoctor)
	payload, _ := json.Marshal(map[string]string{"record": "HeartRate:72BPM,Cadence:110"})
	reqAnchor := httptest.NewRequest("POST", "/api/v1/blockchain/anchor", bytes.NewBuffer(payload))
	reqAnchor.Header.Set("Authorization", "Bearer "+doctorToken)
	wAnchor := httptest.NewRecorder()
	gateway.withRBAC(RoleDoctor, gateway.handleAnchor)(wAnchor, reqAnchor)

	if wAnchor.Code != http.StatusOK {
		t.Errorf("Expected 200, got %d", wAnchor.Code)
	}

	var resp ResponseEnvelope
	if err := json.NewDecoder(wAnchor.Body).Decode(&resp); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}
	if resp.Code != 0 {
		t.Errorf("Expected response code 0, got %d", resp.Code)
	}

	// 7. Test Cast Endpoint with User Role Token
	castPayload, _ := json.Marshal(map[string]string{"target_ip": "192.168.1.10", "media_url": "http://localhost/view"})
	reqCast := httptest.NewRequest("POST", "/api/v1/ui/cast", bytes.NewBuffer(castPayload))
	reqCast.Header.Set("Authorization", "Bearer "+userToken)
	wCast := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleCast)(wCast, reqCast)
	if wCast.Code != http.StatusOK {
		t.Errorf("Expected 200 on cast, got %d", wCast.Code)
	}

	// 8. Test Redeem Endpoint with User Role Token
	redeemPayload, _ := json.Marshal(map[string]interface{}{"client_ip": "192.168.1.5", "client_mac": "AA:BB:CC:DD:EE:FF", "coins": 100})
	reqRedeem := httptest.NewRequest("POST", "/api/v1/monetization/redeem", bytes.NewBuffer(redeemPayload))
	reqRedeem.Header.Set("Authorization", "Bearer "+userToken)
	wRedeem := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleRedeem)(wRedeem, reqRedeem)
	if wRedeem.Code != http.StatusOK {
		t.Errorf("Expected 200 on redeem, got %d", wRedeem.Code)
	}

	// 9. Test HL7 FHIR Export Endpoint with Doctor Role Token
	reqFHIR := httptest.NewRequest("GET", "/api/v1/medical/fhir_export", nil)
	reqFHIR.Header.Set("Authorization", "Bearer "+doctorToken)
	wFHIR := httptest.NewRecorder()
	gateway.withRBAC(RoleDoctor, gateway.handleFHIRExport)(wFHIR, reqFHIR)
	if wFHIR.Code != http.StatusOK {
		t.Errorf("Expected 200 on FHIR export, got %d", wFHIR.Code)
	}

	// 10. Test Generative AI Rehab Plan Endpoint with User Role Token
	aiPayload, _ := json.Marshal(map[string]interface{}{
		"user_age":       78,
		"avg_heart_rate": 85.5,
		"recent_fall":    true,
		"target_goal":    "Improve balance & gait stability",
	})
	reqAI := httptest.NewRequest("POST", "/api/v1/ai/rehab_plan", bytes.NewBuffer(aiPayload))
	reqAI.Header.Set("Authorization", "Bearer "+userToken)
	wAI := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleGenerativeRehabPlan)(wAI, reqAI)
	if wAI.Code != http.StatusOK {
		t.Errorf("Expected 200 on AI rehab plan, got %d", wAI.Code)
	}

	// 11. Test Family Album Upload Endpoint with Family Role Token
	familyToken, _ := gateway.generateToken("daughter_lily", RoleFamily)
	albumPayload, _ := json.Marshal(map[string]string{
		"family_member": "Lily",
		"caption":       "Grandma at Central Park",
		"image_base64":  "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=",
	})
	reqAlbum := httptest.NewRequest("POST", "/api/v1/album/upload", bytes.NewBuffer(albumPayload))
	reqAlbum.Header.Set("Authorization", "Bearer "+familyToken)
	wAlbum := httptest.NewRecorder()
	gateway.withRBAC(RoleFamily, gateway.handleAlbumUpload)(wAlbum, reqAlbum)
	if wAlbum.Code != http.StatusOK {
		t.Errorf("Expected 200 on album upload, got %d", wAlbum.Code)
	}

	// 12. Test Voice Command Parser Endpoint with User Role Token
	voicePayload, _ := json.Marshal(map[string]string{"transcript": "启动步行训练"})
	reqVoice := httptest.NewRequest("POST", "/api/v1/voice/command", bytes.NewBuffer(voicePayload))
	reqVoice.Header.Set("Authorization", "Bearer "+userToken)
	wVoice := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleVoiceCommand)(wVoice, reqVoice)
	if wVoice.Code != http.StatusOK {
		t.Errorf("Expected 200 on voice command, got %d", wVoice.Code)
	}

	// 13. Test Smart Speaker OpenAPI Bridge Endpoint
	speakerPayload, _ := json.Marshal(map[string]interface{}{
		"speaker_type": "xiaomi",
		"action":       "get_status",
	})
	reqSpeaker := httptest.NewRequest("POST", "/api/v1/voice/bridge/speaker", bytes.NewBuffer(speakerPayload))
	reqSpeaker.Header.Set("Authorization", "Bearer "+userToken)
	wSpeaker := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleSpeakerBridge)(wSpeaker, reqSpeaker)
	if wSpeaker.Code != http.StatusOK {
		t.Errorf("Expected 200 on speaker bridge, got %d", wSpeaker.Code)
	}

	// 14. Test Biometric Face Recognition Endpoint
	facePayload, _ := json.Marshal(map[string]string{"frame_base64": "dummy_raw_frame_bytes"})
	reqFace := httptest.NewRequest("POST", "/api/v1/cv/face_verify", bytes.NewBuffer(facePayload))
	reqFace.Header.Set("Authorization", "Bearer "+userToken)
	wFace := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleFaceVerify)(wFace, reqFace)
	if wFace.Code != http.StatusOK {
		t.Errorf("Expected 200 on face verify, got %d", wFace.Code)
	}

	// 15. Test Prescription Validation Endpoint
	valPayload, _ := json.Marshal(map[string]string{
		"item_id":         "ITEM_GAIT_01",
		"prescription_id": "RX-TEST-001",
	})
	reqVal := httptest.NewRequest("POST", "/api/v1/prescription/validate", bytes.NewBuffer(valPayload))
	reqVal.Header.Set("Authorization", "Bearer "+userToken)
	wVal := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handlePrescriptionValidate)(wVal, reqVal)
	if wVal.Code != http.StatusOK {
		t.Errorf("Expected 200 on prescription validate, got %d", wVal.Code)
	}

	// 15b. Test Remote Assistance Web Screen Mirroring Endpoint
	reqScreen := httptest.NewRequest("GET", "/api/v1/remote/screen", nil)
	reqScreen.Header.Set("Authorization", "Bearer "+userToken)
	wScreen := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleRemoteScreen)(wScreen, reqScreen)
	if wScreen.Code != http.StatusOK {
		t.Errorf("Expected 200 on remote screen, got %d", wScreen.Code)
	}

	// 16. Test Log Download Binary Stream Endpoint
	reqLog := httptest.NewRequest("GET", "/api/v1/logs/download", nil)
	reqLog.Header.Set("Authorization", "Bearer "+adminToken)
	wLog := httptest.NewRecorder()
	gateway.withRBAC(RoleAdmin, gateway.handleLogsDownload)(wLog, reqLog)
	if wLog.Code != http.StatusOK {
		t.Errorf("Expected 200 on log download, got %d", wLog.Code)
	}
	if contentType := wLog.Header().Get("Content-Type"); contentType != "application/gzip" {
		t.Errorf("Expected Content-Type application/gzip, got %s", contentType)
	}

	// 17. Test Heartbeat Ticker Reporting
	hb := gateway.sendHeartbeat()
	if hb.SystemStatus != "HEALTHY" || hb.NetStatus != "ONLINE" || hb.BatterySOC <= 0 {
		t.Errorf("Unexpected heartbeat payload: %+v", hb)
	}
}

func TestUDSIPC(t *testing.T) {
	db, err := sql.Open("sqlite3", ":memory:")
	if err != nil {
		t.Fatal(err)
	}
	defer db.Close()

	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS devices (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, dev_type TEXT NOT NULL, slave_id INTEGER DEFAULT 0, protocol TEXT NOT NULL, address TEXT NOT NULL, baud INTEGER DEFAULT 0, bound_time DATETIME DEFAULT CURRENT_TIMESTAMP);")
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS tag_history (timestamp INTEGER, tag_id INTEGER, value REAL, status INTEGER);")

	gateway := &AppGateway{
		db: db,
	}

	c1, c2 := net.Pipe()
	defer c1.Close()
	defer c2.Close()

	go gateway.handleUDSConnection(c1)

	nfcPayload := `{"dev_type":"exoskeleton","slave_id":1,"protocol":"ModbusRTU","baud":115200}`
	length := len(nfcPayload)
	frame := make([]byte, 5+length)
	frame[0] = 0x53
	frame[1] = 0x42
	frame[2] = 2
	frame[3] = byte(length)
	copy(frame[4:], []byte(nfcPayload))

	checksum := byte(0)
	for i := 0; i < 4+length; i++ {
		checksum ^= frame[i]
	}
	frame[4+length] = checksum

	_, err = c2.Write(frame)
	if err != nil {
		t.Fatalf("Failed to write to pipe: %v", err)
	}

	time.Sleep(10 * time.Millisecond)

	var devType, protocol string
	var slaveID, baud int
	err = db.QueryRow("SELECT dev_type, slave_id, protocol, baud FROM devices").Scan(&devType, &slaveID, &protocol, &baud)
	if err != nil {
		t.Fatalf("Failed to query devices: %v", err)
	}

	if devType != "exoskeleton" || slaveID != 1 || protocol != "ModbusRTU" || baud != 115200 {
		t.Errorf("Device register mismatch: devType=%q, slaveID=%d, protocol=%q, baud=%d", devType, slaveID, protocol, baud)
	}

	c3, c4 := net.Pipe()
	defer c3.Close()
	defer c4.Close()

	go gateway.handleUDSConnection(c3)

	sosFrame := make([]byte, 5)
	sosFrame[0] = 0x53
	sosFrame[1] = 0x42
	sosFrame[2] = 3
	sosFrame[3] = 0
	sosFrame[4] = 0x53 ^ 0x42 ^ 3 ^ 0

	_, err = c4.Write(sosFrame)
	if err != nil {
		t.Fatalf("Failed to write to pipe: %v", err)
	}

	time.Sleep(10 * time.Millisecond)

	var tagID int
	var val float64
	err = db.QueryRow("SELECT tag_id, value FROM tag_history").Scan(&tagID, &val)
	if err != nil {
		t.Fatalf("Failed to query tag history: %v", err)
	}
	if tagID != 12003 || val != 1.0 {
		t.Errorf("SOS tag history mismatch: tagID=%d, val=%.2f", tagID, val)
	}
}
