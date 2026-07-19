package main

import (
	"bytes"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"database/sql"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"

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
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS blockchain_receipts (timestamp INTEGER PRIMARY KEY, tx_hash TEXT, record_hash TEXT, signature TEXT);")

	priv, _ := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	gateway := &AppGateway{
		db:      db,
		aesKey:  make([]byte, 32),
		privKey: priv,
	}
	rand.Read(gateway.aesKey)

	// Test saveSensitiveData and decryption
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

	// Test Anchor REST Endpoint
	payload, _ := json.Marshal(map[string]string{"record": "HeartRate:72BPM,Cadence:110"})
	req := httptest.NewRequest("POST", "/api/v1/blockchain/anchor", bytes.NewBuffer(payload))
	w := httptest.NewRecorder()
	gateway.handleAnchor(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected 200, got %d", w.Code)
	}

	var resp ResponseEnvelope
	if err := json.NewDecoder(w.Body).Decode(&resp); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}
	if resp.Code != 0 {
		t.Errorf("Expected response code 0, got %d", resp.Code)
	}

	// Test Purge Endpoint
	reqPurge := httptest.NewRequest("POST", "/api/v1/privacy/purge", nil)
	wPurge := httptest.NewRecorder()
	gateway.handlePurge(wPurge, reqPurge)

	if wPurge.Code != http.StatusOK {
		t.Errorf("Expected 200 on purge, got %d", wPurge.Code)
	}

	// Test Cast Endpoint
	castPayload, _ := json.Marshal(map[string]string{"target_ip": "192.168.1.10", "url": "http://localhost/view"})
	reqCast := httptest.NewRequest("POST", "/api/v1/ui/cast", bytes.NewBuffer(castPayload))
	wCast := httptest.NewRecorder()
	gateway.handleCast(wCast, reqCast)
	if wCast.Code != http.StatusOK {
		t.Errorf("Expected 200 on cast, got %d", wCast.Code)
	}

	// Test Redeem Endpoint
	redeemPayload, _ := json.Marshal(map[string]interface{}{"client_ip": "192.168.1.5", "client_mac": "AA:BB:CC:DD:EE:FF", "coins": 100})
	reqRedeem := httptest.NewRequest("POST", "/api/v1/monetization/redeem", bytes.NewBuffer(redeemPayload))
	wRedeem := httptest.NewRecorder()
	gateway.handleRedeem(wRedeem, reqRedeem)
	if wRedeem.Code != http.StatusOK {
		t.Errorf("Expected 200 on redeem, got %d", wRedeem.Code)
	}
}
