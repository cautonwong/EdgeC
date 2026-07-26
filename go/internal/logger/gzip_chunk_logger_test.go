package logger

import (
	"compress/gzip"
	"io"
	"os"
	"path/filepath"
	"testing"
)

func TestGzipChunkLogger(t *testing.T) {
	tempDir, err := os.MkdirTemp("", "edge_log_test")
	if err != nil {
		t.Fatalf("Failed to create temp dir: %v", err)
	}
	defer os.RemoveAll(tempDir)

	cl, err := NewChunkLogger(tempDir, 1) // 1KB threshold
	if err != nil {
		t.Fatalf("Failed to create ChunkLogger: %v", err)
	}

	// Write enough log lines to trigger a chunk flush
	for i := 0; i < 50; i++ {
		err := cl.WriteLog("SmartBox continuous rehabilitation telemetry data log entry test")
		if err != nil {
			t.Fatalf("WriteLog failed: %v", err)
		}
	}

	cl.Close()

	files, err := filepath.Glob(filepath.Join(tempDir, "*.log.gz"))
	if err != nil || len(files) == 0 {
		t.Fatalf("Expected at least 1 gzipped chunk log file, found %d", len(files))
	}

	// Verify file is valid gzip and readable
	f, err := os.Open(files[0])
	if err != nil {
		t.Fatalf("Failed to open gz log file: %v", err)
	}
	defer f.Close()

	gr, err := gzip.NewReader(f)
	if err != nil {
		t.Fatalf("Failed to create gzip reader: %v", err)
	}
	defer gr.Close()

	content, err := io.ReadAll(gr)
	if err != nil {
		t.Fatalf("Failed to read decompressed gz log content: %v", err)
	}

	if len(content) == 0 {
		t.Fatal("Decompressed log content should not be empty")
	}

	t.Logf("Successfully read %d bytes of decompressed log content from %s", len(content), files[0])
}
