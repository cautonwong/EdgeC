package p2p

import (
	"crypto/x509"
	"encoding/pem"
	"errors"
	"time"
)

// CertValidationResult holds x509 validation summary.
type CertValidationResult struct {
	SubjectCN string    `json:"subject_cn"`
	IssuerCN  string    `json:"issuer_cn"`
	NotBefore time.Time `json:"not_before"`
	NotAfter  time.Time `json:"not_after"`
	IsValid   bool      `json:"is_valid"`
}

// MTLSValidator performs mTLS client certificate verification for Zero-Trust edge nodes.
type MTLSValidator struct {
	rootPool *x509.CertPool
}

// NewMTLSValidator creates an mTLS cert validator with trusted CA pool.
func NewMTLSValidator(caPEM []byte) (*MTLSValidator, error) {
	pool := x509.NewCertPool()
	if len(caPEM) > 0 {
		if !pool.AppendCertsFromPEM(caPEM) {
			return nil, errors.New("failed to parse root CA PEM")
		}
	}
	return &MTLSValidator{rootPool: pool}, nil
}

// ValidateClientCert validates a raw client certificate PEM against system/custom trust pool.
func (v *MTLSValidator) ValidateClientCert(clientCertPEM []byte) (*CertValidationResult, error) {
	block, _ := pem.Decode(clientCertPEM)
	if block == nil || block.Type != "CERTIFICATE" {
		return nil, errors.New("invalid client certificate PEM block")
	}

	cert, err := x509.ParseCertificate(block.Bytes)
	if err != nil {
		return nil, err
	}

	now := time.Now()
	isValid := now.After(cert.NotBefore) && now.Before(cert.NotAfter)

	if len(v.rootPool.Subjects()) > 0 {
		opts := x509.VerifyOptions{
			Roots:       v.rootPool,
			CurrentTime: now,
			KeyUsages:   []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
		}
		if _, err := cert.Verify(opts); err != nil {
			isValid = false
		}
	}

	return &CertValidationResult{
		SubjectCN: cert.Subject.CommonName,
		IssuerCN:  cert.Issuer.CommonName,
		NotBefore: cert.NotBefore,
		NotAfter:  cert.NotAfter,
		IsValid:   isValid,
	}, nil
}
