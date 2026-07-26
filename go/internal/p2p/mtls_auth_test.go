package p2p

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"math/big"
	"testing"
	"time"
)

func generateTestCert(t *testing.T) ([]byte, []byte) {
	priv, err := rsa.GenerateKey(rand.Reader, 2048)
	if err != nil {
		t.Fatalf("failed to generate key: %v", err)
	}

	template := x509.Certificate{
		SerialNumber: big.NewInt(1001),
		Subject: pkix.Name{
			CommonName:   "EdgeNode-SmartBox-01",
			Organization: []string{"EdgeOS Security"},
		},
		Issuer: pkix.Name{
			CommonName: "EdgeOS Root CA",
		},
		NotBefore:             time.Now().Add(-1 * time.Hour),
		NotAfter:              time.Now().Add(24 * time.Hour),
		KeyUsage:              x509.KeyUsageDigitalSignature,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
		BasicConstraintsValid: true,
	}

	certBytes, err := x509.CreateCertificate(rand.Reader, &template, &template, &priv.PublicKey, priv)
	if err != nil {
		t.Fatalf("failed to create cert: %v", err)
	}

	certPEM := pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: certBytes})
	return certPEM, certPEM
}

func TestMTLSValidator(t *testing.T) {
	caPEM, clientPEM := generateTestCert(t)

	validator, err := NewMTLSValidator(caPEM)
	if err != nil {
		t.Fatalf("failed to create validator: %v", err)
	}

	res, err := validator.ValidateClientCert(clientPEM)
	if err != nil {
		t.Fatalf("validation failed with error: %v", err)
	}

	if res.SubjectCN != "EdgeNode-SmartBox-01" {
		t.Errorf("expected CN EdgeNode-SmartBox-01, got %s", res.SubjectCN)
	}
	if !res.IsValid {
		t.Errorf("expected cert to be valid")
	}
}
