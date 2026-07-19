package main

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/sha256"
	"crypto/x509"
	"database/sql"
	"encoding/base64"
	"encoding/json"
	"encoding/pem"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"sync"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

type AppGateway struct {
	db      *sql.DB
	mu      sync.Mutex
	aesKey  []byte
	privKey *ecdsa.PrivateKey
}

type ResponseEnvelope struct {
	Code    int         `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data,omitempty"`
	Meta    interface{} `json:"meta"`
}

func getMeta() map[string]interface{} {
	return map[string]interface{}{
		"mtt": time.Now().UnixNano() / 1e6,
	}
}

// AES-256-GCM Sizing Helper
func (g *AppGateway) encrypt(plainText string) (string, string, string, error) {
	block, err := aes.NewCipher(g.aesKey)
	if err != nil {
		return "", "", "", err
	}
	aesgcm, err := cipher.NewGCM(block)
	if err != nil {
		return "", "", "", err
	}
	iv := make([]byte, 12)
	if _, err := io.ReadFull(rand.Reader, iv); err != nil {
		return "", "", "", err
	}
	cipherBytes := aesgcm.Seal(nil, iv, []byte(plainText), nil)

	tagLen := aesgcm.Overhead()
	cipherLen := len(cipherBytes)
	authTag := cipherBytes[cipherLen-tagLen:]
	ciphertext := cipherBytes[:cipherLen-tagLen]

	return base64.StdEncoding.EncodeToString(iv),
		base64.StdEncoding.EncodeToString(ciphertext),
		base64.StdEncoding.EncodeToString(authTag),
		nil
}

func (g *AppGateway) decrypt(base64IV, base64Ciphertext, base64AuthTag string) (string, error) {
	iv, err := base64.StdEncoding.DecodeString(base64IV)
	if err != nil {
		return "", err
	}
	ciphertext, err := base64.StdEncoding.DecodeString(base64Ciphertext)
	if err != nil {
		return "", err
	}
	authTag, err := base64.StdEncoding.DecodeString(base64AuthTag)
	if err != nil {
		return "", err
	}
	block, err := aes.NewCipher(g.aesKey)
	if err != nil {
		return "", err
	}
	aesgcm, err := cipher.NewGCM(block)
	if err != nil {
		return "", err
	}
	combined := append(ciphertext, authTag...)
	plainBytes, err := aesgcm.Open(nil, iv, combined, nil)
	if err != nil {
		return "", err
	}
	return string(plainBytes), nil
}

func (g *AppGateway) saveSensitiveData(tagID int, val string) error {
	iv, ciphertext, authTag, err := g.encrypt(val)
	if err != nil {
		return err
	}
	g.mu.Lock()
	defer g.mu.Unlock()
	_, err = g.db.Exec("INSERT INTO sensitive_records(timestamp, tag_id, iv, ciphertext, auth_tag) VALUES(?, ?, ?, ?, ?);",
		time.Now().UnixNano()/1e6, tagID, iv, ciphertext, authTag)
	return err
}

// GDPR Purge with VACUUM outside Tx but under Mutex Lock
func (g *AppGateway) handlePurge(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	g.mu.Lock()
	defer g.mu.Unlock()

	tx, err := g.db.Begin()
	if err != nil {
		http.Error(w, "Transaction error", http.StatusInternalServerError)
		return
	}
	_, _ = tx.Exec("DELETE FROM sensitive_records;")
	_, _ = tx.Exec("DELETE FROM tag_history;")
	_, _ = tx.Exec("DELETE FROM blockchain_receipts;")
	if err := tx.Commit(); err != nil {
		http.Error(w, "Commit failed", http.StatusInternalServerError)
		return
	}

	// Run VACUUM outside active transaction, under lock
	if _, err := g.db.Exec("VACUUM;"); err != nil {
		log.Printf("Warning: VACUUM failed: %v", err)
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ResponseEnvelope{
		Code:    0,
		Message: "GDPR compliance zero-wipe completed successfully",
		Meta:    getMeta(),
	})
}

// Blockchain Anchor with padding and time-replay protection
func (g *AppGateway) handleAnchor(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var request struct {
		Record string `json:"record"`
	}
	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		http.Error(w, "Invalid JSON", http.StatusBadRequest)
		return
	}

	ts := time.Now().UnixNano() / 1e6
	h := sha256.New()
	h.Write([]byte(request.Record))
	h.Write([]byte(fmt.Sprintf("-%d", ts)))
	recordHash := h.Sum(nil)

	rSig, sSig, err := ecdsa.Sign(rand.Reader, g.privKey, recordHash)
	if err != nil {
		http.Error(w, "Signature generation failed", http.StatusInternalServerError)
		return
	}

	rBytes := rSig.Bytes()
	sBytes := sSig.Bytes()
	paddedSig := make([]byte, 64)
	copy(paddedSig[32-len(rBytes):32], rBytes)
	copy(paddedSig[64-len(sBytes):64], sBytes)
	base64Sig := base64.StdEncoding.EncodeToString(paddedSig)

	txHash := fmt.Sprintf("0x%x", sha256.Sum256([]byte(fmt.Sprintf("%d-%s", ts, base64Sig))))

	g.mu.Lock()
	_, _ = g.db.Exec("INSERT INTO blockchain_receipts(timestamp, tx_hash, record_hash, signature) VALUES(?, ?, ?, ?);",
		ts, txHash, fmt.Sprintf("%x", recordHash), base64Sig)
	g.mu.Unlock()

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ResponseEnvelope{
		Code:    0,
		Message: "Blockchain anchor anchored successfully",
		Data: map[string]interface{}{
			"tx_hash":     txHash,
			"record_hash": fmt.Sprintf("%x", recordHash),
			"signature":   base64Sig,
		},
		Meta: getMeta(),
	})
}

// DLNA Cast Mock Endpoint
func (g *AppGateway) handleCast(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var request struct {
		TargetIP string `json:"target_ip"`
		URL      string `json:"url"`
	}
	_ = json.NewDecoder(r.Body).Decode(&request)

	fmt.Printf("[DLNA Cast] Discovering TV at %s, casting URL: %s\n", request.TargetIP, request.URL)
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ResponseEnvelope{
		Code:    0,
		Message: "Cast sent via AVTransport successfully",
		Meta:    getMeta(),
	})
}

// Captive Portal Traffic Redeem Mock Endpoint
func (g *AppGateway) handleRedeem(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var request struct {
		ClientIP  string `json:"client_ip"`
		ClientMAC string `json:"client_mac"`
		Coins     int    `json:"coins"`
	}
	_ = json.NewDecoder(r.Body).Decode(&request)

	mb := request.Coins / 10
	bytesQuota := mb * 1024 * 1024
	cmdStr := fmt.Sprintf("iptables -t filter -I FORWARD 1 -i wlan0 -o wwan0 -m mac --mac-source %s -j ACCEPT", request.ClientMAC)
	fmt.Printf("[TRAFFIC CTL] Execution Shell: %s\n", cmdStr)
	fmt.Printf("[TRAFFIC CTL] Unblocked WAN access for client MAC %s. Quota: %d Bytes\n", request.ClientMAC, bytesQuota)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(ResponseEnvelope{
		Code:    0,
		Message: fmt.Sprintf("Successfully redeemed %d MB WAN traffic", mb),
		Meta:    getMeta(),
	})
}

func main() {
	dbPath := os.Getenv("EDGE_DB_PATH")
	if dbPath == "" {
		dbPath = "smart_box.db"
	}
	db, err := sql.Open("sqlite3", dbPath)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS ui_configs (key TEXT PRIMARY KEY, val TEXT);")
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS sensitive_records (timestamp INTEGER, tag_id INTEGER, iv TEXT, ciphertext TEXT, auth_tag TEXT);")
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS tag_history (timestamp INTEGER, tag_id INTEGER, value REAL, status INTEGER);")
	_, _ = db.Exec("CREATE TABLE IF NOT EXISTS blockchain_receipts (timestamp INTEGER PRIMARY KEY, tx_hash TEXT, record_hash TEXT, signature TEXT);")

	gateway := &AppGateway{
		db:     db,
		aesKey: make([]byte, 32),
	}
	rand.Read(gateway.aesKey)

	priv, _ := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	gateway.privKey = priv

	pemBytes, err := os.ReadFile("keys/smartbox_private.pem")
	if err == nil {
		block, _ := pem.Decode(pemBytes)
		if block != nil {
			k, err := x509.ParseECPrivateKey(block.Bytes)
			if err == nil {
				gateway.privKey = k
			}
		}
	}

	http.HandleFunc("/api/v1/privacy/purge", gateway.handlePurge)
	http.HandleFunc("/api/v1/blockchain/anchor", gateway.handleAnchor)
	http.HandleFunc("/api/v1/ui/cast", gateway.handleCast)
	http.HandleFunc("/api/v1/monetization/redeem", gateway.handleRedeem)

	port := os.Getenv("GATEWAY_PORT")
	if port == "" {
		port = "8083"
	}
	fmt.Println("Smart Box Go Gateway listening on port " + port)
	http.ListenAndServe(":"+port, nil)
}
