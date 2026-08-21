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
	"os"
	"testing"
	"time"

	"edgeos.go/internal/prescription"
	"edgeos.go/pkg/shm"
	"edgeos.go/pkg/tags"
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

func TestUDSSecurityHardeningAndRBAC(t *testing.T) {
	db, err := sql.Open("sqlite3", ":memory:")
	if err != nil {
		t.Fatal(err)
	}
	defer db.Close()
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS tag_history (timestamp INTEGER, tag_id INTEGER, value REAL, status INTEGER);")

	gateway := &AppGateway{
		db:        db,
		jwtSecret: make([]byte, 32),
	}
	rand.Read(gateway.jwtSecret)

	userToken, _ := gateway.generateToken("user1", RoleUser)
	adminToken, _ := gateway.generateToken("admin1", RoleAdmin)

	// 1. Test OTA Check RBAC Protection
	reqOTAUnauth := httptest.NewRequest("GET", "/api/v1/device/ota/check", nil)
	wOTAUnauth := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleDeviceOTACheck)(wOTAUnauth, reqOTAUnauth)
	if wOTAUnauth.Code != http.StatusUnauthorized {
		t.Errorf("Expected 401 for unauth OTA check, got %d", wOTAUnauth.Code)
	}

	reqOTAAuth := httptest.NewRequest("GET", "/api/v1/device/ota/check", nil)
	reqOTAAuth.Header.Set("Authorization", "Bearer "+userToken)
	wOTAAuth := httptest.NewRecorder()
	gateway.withRBAC(RoleUser, gateway.handleDeviceOTACheck)(wOTAAuth, reqOTAAuth)
	if wOTAAuth.Code != http.StatusOK {
		t.Errorf("Expected 200 for auth OTA check, got %d", wOTAAuth.Code)
	}

	// 2. Test UDS Security REST Endpoint Role Protection
	reqSecUser := httptest.NewRequest("GET", "/api/v1/ecu/uds/security", nil)
	reqSecUser.Header.Set("Authorization", "Bearer "+userToken)
	wSecUser := httptest.NewRecorder()
	gateway.withRBAC(RoleAdmin, gateway.handleUDSSecurity)(wSecUser, reqSecUser)
	if wSecUser.Code != http.StatusForbidden {
		t.Errorf("Expected 403 for user accessing UDS security endpoint, got %d", wSecUser.Code)
	}

	reqSecAdmin := httptest.NewRequest("GET", "/api/v1/ecu/uds/security", nil)
	reqSecAdmin.Header.Set("Authorization", "Bearer "+adminToken)
	wSecAdmin := httptest.NewRecorder()
	gateway.withRBAC(RoleAdmin, gateway.handleUDSSecurity)(wSecAdmin, reqSecAdmin)
	if wSecAdmin.Code != http.StatusOK {
		t.Errorf("Expected 200 for admin accessing UDS security endpoint, got %d", wSecAdmin.Code)
	}

	// 3. Test SecurityAccess 0x27 Seed & Key Logic over DoIP handler
	c1, c2 := net.Pipe()
	defer c1.Close()
	defer c2.Close()

	go gateway.handleDoIPConnection(c1)

	// Step A: Routing Activation (0x0005)
	actReq := []byte{0x02, 0xF9, 0x00, 0x05, 0x00, 0x00, 0x00, 0x07, 0x0E, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00}
	c2.Write(actReq)
	actResp := make([]byte, 19)
	c2.Read(actResp)

	// Step B: Request Seed (0x27 0x01)
	reqSeedBody := []byte{0x00, 0x0E, 0x00, 0x10, 0x27, 0x01}
	payLen := uint32(len(reqSeedBody))
	reqSeedMsg := append([]byte{0x02, 0xF9, 0x80, 0x01, byte(payLen >> 24), byte(payLen >> 16), byte(payLen >> 8), byte(payLen)}, reqSeedBody...)
	c2.Write(reqSeedMsg)

	respBuf := make([]byte, 256)
	n, _ := c2.Read(respBuf)
	if n < 14 || respBuf[12] != 0x67 || respBuf[13] != 0x01 {
		t.Fatalf("Failed to receive seed, response len: %d, bytes: %x", n, respBuf[:n])
	}
	seed := append([]byte(nil), respBuf[14:18]...)

	// Step C: Send Wrong Key (0x27 0x02)
	wrongKey := []byte{0xFF, 0xFF, 0xFF, 0xFF}
	sendWrongBody := append([]byte{0x00, 0x0E, 0x00, 0x10, 0x27, 0x02}, wrongKey...)
	payLenW := uint32(len(sendWrongBody))
	sendWrongMsg := append([]byte{0x02, 0xF9, 0x80, 0x01, byte(payLenW >> 24), byte(payLenW >> 16), byte(payLenW >> 8), byte(payLenW)}, sendWrongBody...)
	c2.Write(sendWrongMsg)

	n, _ = c2.Read(respBuf)
	if n < 15 || respBuf[12] != 0x7F || respBuf[14] != 0x35 {
		t.Errorf("Expected negative response 0x7F 0x27 0x35 for wrong key, got: %x", respBuf[:n])
	}

	// Step D: Send Correct Key
	correctKey := computeSecurityKey(seed)
	sendRightBody := append([]byte{0x00, 0x0E, 0x00, 0x10, 0x27, 0x02}, correctKey...)
	payLenR := uint32(len(sendRightBody))
	sendRightMsg := append([]byte{0x02, 0xF9, 0x80, 0x01, byte(payLenR >> 24), byte(payLenR >> 16), byte(payLenR >> 8), byte(payLenR)}, sendRightBody...)
	c2.Write(sendRightMsg)

	n, _ = c2.Read(respBuf)
	if n < 14 || respBuf[12] != 0x67 || respBuf[13] != 0x02 {
		t.Errorf("Expected positive response 0x67 0x02 for correct key, got: %x", respBuf[:n])
	}

	// Step E: Test DiagnosticSessionControl (0x10)
	sessionReq := []byte{0x02, 0xF9, 0x80, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0E, 0x00, 0x10, 0x10, 0x03}
	c2.Write(sessionReq)
	n, _ = c2.Read(respBuf)
	if n < 14 || respBuf[12] != 0x50 || respBuf[13] != 0x03 {
		t.Errorf("Expected positive session response 0x50 0x03, got: %x", respBuf[:n])
	}

	// Step F: Test ReadDataByIdentifier (0x22)
	readReq := []byte{0x02, 0xF9, 0x80, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x0E, 0x00, 0x10, 0x22, 0xF1, 0x90}
	c2.Write(readReq)
	n, _ = c2.Read(respBuf)
	if n < 15 || respBuf[12] != 0x62 {
		t.Errorf("Expected positive read DID response 0x62, got: %x", respBuf[:n])
	}

	// Step G: Test ReadDTCInformation (0x19)
	dtcReq := []byte{0x02, 0xF9, 0x80, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0E, 0x00, 0x10, 0x19, 0x02}
	c2.Write(dtcReq)
	n, _ = c2.Read(respBuf)
	if n < 14 || respBuf[12] != 0x59 {
		t.Errorf("Expected positive DTC response 0x59, got: %x", respBuf[:n])
	}

	// Step H: Test RoutineControl (0x31)
	routineReq := []byte{0x02, 0xF9, 0x80, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0E, 0x00, 0x10, 0x31, 0x01, 0x02, 0x01}
	c2.Write(routineReq)
	n, _ = c2.Read(respBuf)
	if n < 14 || respBuf[12] != 0x71 {
		t.Errorf("Expected positive Routine response 0x71, got: %x", respBuf[:n])
	}

	// Verify DataHub SQLite audit records were written
	var count int
	_ = db.QueryRow("SELECT COUNT(*) FROM tag_history WHERE tag_id = 12034").Scan(&count)
	if count == 0 {
		t.Errorf("Expected tag_history record for UDS_SECURITY_UNLOCKED (12034)")
	}
}

func TestUDSProductionSecretFailFast(t *testing.T) {
	// 1. In production mode, empty secret MUST return fatal error
	_, err := validateAndGetUDSSecret("", "production")
	if err == nil {
		t.Errorf("Expected error when DOIP_UDS_SECRET_KEY is empty in production mode, got nil")
	}
	_, err2 := validateAndGetUDSSecret("", "prod")
	if err2 == nil {
		t.Errorf("Expected error when DOIP_UDS_SECRET_KEY is empty in prod mode, got nil")
	}

	// 2. In non-production mode, empty secret returns ephemeral secret without error
	devSec, err3 := validateAndGetUDSSecret("", "development")
	if err3 != nil || len(devSec) != 32 {
		t.Errorf("Expected 32-byte ephemeral secret in dev mode, got %v, len=%d", err3, len(devSec))
	}

	// 3. When explicit secret is provided, returns configured secret
	cfgSec, err4 := validateAndGetUDSSecret("MyCustomSecret123", "production")
	if err4 != nil || string(cfgSec) != "MyCustomSecret123" {
		t.Errorf("Expected configured secret, got %v (%s)", err4, string(cfgSec))
	}
}

func TestUDSDynamicDTCConfiguration(t *testing.T) {
	// A. Test when unconfigured -> returns empty / no data
	os.Unsetenv("UDS_ACTIVE_DTCS")
	dtcs := getActiveDTCRecords()
	if len(dtcs) != 0 {
		t.Errorf("Expected 0 DTC records when unconfigured, got %d", len(dtcs))
	}

	// Test REST endpoint when unconfigured
	gw := &AppGateway{jwtSecret: make([]byte, 32)}
	w := httptest.NewRecorder()
	r := httptest.NewRequest("GET", "/api/v1/ecu/uds/dtc", nil)
	gw.handleUDSDTC(w, r)
	if w.Code != http.StatusOK {
		t.Errorf("Expected 200 OK, got %d", w.Code)
	}
	var resp map[string]interface{}
	if err := json.Unmarshal(w.Body.Bytes(), &resp); err != nil {
		t.Fatalf("Failed to parse JSON response: %v", err)
	}
	data := resp["data"].(map[string]interface{})
	if data["active_dtc_count"].(float64) != 0 {
		t.Errorf("Expected active_dtc_count=0 when unconfigured, got %v", data["active_dtc_count"])
	}

	// B. Test when configured dynamically via JSON env
	cfg := `[{"dtc_code":"P0101","description":"Mass Air Flow Circuit Fault","status":"CONFIRMED","raw_bytes":[1,1,0,1]}]`
	os.Setenv("UDS_ACTIVE_DTCS", cfg)
	defer os.Unsetenv("UDS_ACTIVE_DTCS")

	dtcsConfigured := getActiveDTCRecords()
	if len(dtcsConfigured) != 1 || dtcsConfigured[0].Code != "P0101" {
		t.Errorf("Expected 1 configured DTC P0101, got %v", dtcsConfigured)
	}

	w2 := httptest.NewRecorder()
	r2 := httptest.NewRequest("GET", "/api/v1/ecu/uds/dtc", nil)
	gw.handleUDSDTC(w2, r2)
	if w2.Code != http.StatusOK {
		t.Errorf("Expected 200 OK, got %d", w2.Code)
	}
	var resp2 map[string]interface{}
	if err := json.Unmarshal(w2.Body.Bytes(), &resp2); err != nil {
		t.Fatalf("Failed to parse JSON response: %v", err)
	}
	data2 := resp2["data"].(map[string]interface{})
	if data2["active_dtc_count"].(float64) != 1 {
		t.Errorf("Expected active_dtc_count=1 when configured, got %v", data2["active_dtc_count"])
	}
}

func TestUDSStateSHMReadback(t *testing.T) {
	matrix, err := shm.CreateOrOpenShadowMatrix("smart_box", 16384)
	if err != nil {
		t.Fatalf("SHM ShadowMatrix create/open failed: %v", err)
	}
	defer func() {
		matrix.Close()
		os.Remove("/dev/shm/edgeos_shadow_smart_box")
	}()

	gw := &AppGateway{matrix: matrix}
	// Write UDS state
	gw.recordTagValue(tags.UDS_SESSION_STATE, 3.0)
	gw.recordTagValue(tags.UDS_SECURITY_UNLOCKED, 1.0)

	// Read back from SHM with hard assertions (0 soft skips)
	msgSession, err := matrix.ReadTag(tags.UDS_SESSION_STATE)
	if err != nil {
		t.Fatalf("ReadTag UDS_SESSION_STATE failed: %v", err)
	}
	val, _ := msgSession.Scalar()
	if val != 3.0 {
		t.Fatalf("Hard assertion failed: expected UDS_SESSION_STATE=3.0, got %f", val)
	}

	msgSec, err := matrix.ReadTag(tags.UDS_SECURITY_UNLOCKED)
	if err != nil {
		t.Fatalf("ReadTag UDS_SECURITY_UNLOCKED failed: %v", err)
	}
	valSec, _ := msgSec.Scalar()
	if valSec != 1.0 {
		t.Fatalf("Hard assertion failed: expected UDS_SECURITY_UNLOCKED=1.0, got %f", valSec)
	}
}

