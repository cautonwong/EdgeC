package platformbackup

import (
	"crypto/rand"
	"encoding/hex"
	"os"
	"path/filepath"
)

// RotateTokenFile replaces the platform's session token file with a fresh
// random value. The temp file plus rename keeps readers from seeing a partial
// token, and mode 0600 keeps it owner-only.
func RotateTokenFile(path string) error {
	buf := make([]byte, 32)
	if _, err := rand.Read(buf); err != nil {
		return err
	}
	tmp, err := os.CreateTemp(filepath.Dir(path), ".session-token-*")
	if err != nil {
		return err
	}
	defer os.Remove(tmp.Name())
	if err := tmp.Chmod(0600); err != nil {
		tmp.Close()
		return err
	}
	if _, err := tmp.WriteString(hex.EncodeToString(buf) + "\n"); err != nil {
		tmp.Close()
		return err
	}
	if err := tmp.Sync(); err != nil {
		tmp.Close()
		return err
	}
	if err := tmp.Close(); err != nil {
		return err
	}
	return os.Rename(tmp.Name(), path)
}
