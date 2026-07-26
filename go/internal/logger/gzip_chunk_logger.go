package logger

import (
	"bytes"
	"compress/gzip"
	"fmt"
	"os"
	"path/filepath"
	"sync"
	"time"
)

type ChunkLogger struct {
	logDir       string
	buf          *bytes.Buffer
	mu           sync.Mutex
	maxChunkSize int
	flushInterval time.Duration
	stopChan     chan struct{}
}

func NewChunkLogger(logDir string, maxChunkKB int) (*ChunkLogger, error) {
	if err := os.MkdirAll(logDir, 0755); err != nil {
		return nil, fmt.Errorf("mkdir log dir failed: %w", err)
	}

	cl := &ChunkLogger{
		logDir:        logDir,
		buf:           bytes.NewBuffer(make([]byte, 0, maxChunkKB*1024)),
		maxChunkSize:  maxChunkKB * 1024,
		flushInterval: 30 * time.Second,
		stopChan:      make(chan struct{}),
	}

	go cl.periodicFlush()
	return cl, nil
}

func (cl *ChunkLogger) WriteLog(line string) error {
	cl.mu.Lock()
	defer cl.mu.Unlock()

	timestamp := time.Now().Format(time.RFC3339)
	entry := fmt.Sprintf("[%s] %s\n", timestamp, line)
	cl.buf.WriteString(entry)

	if cl.buf.Len() >= cl.maxChunkSize {
		return cl.flushBufferLocked()
	}
	return nil
}

func (cl *ChunkLogger) Flush() error {
	cl.mu.Lock()
	defer cl.mu.Unlock()
	return cl.flushBufferLocked()
}

func (cl *ChunkLogger) Close() error {
	close(cl.stopChan)
	return cl.Flush()
}

func (cl *ChunkLogger) flushBufferLocked() error {
	if cl.buf.Len() == 0 {
		return nil
	}

	filename := fmt.Sprintf("edge_chunk_%s.log.gz", time.Now().Format("20060102_150405_999"))
	filePath := filepath.Join(cl.logDir, filename)

	f, err := os.Create(filePath)
	if err != nil {
		return fmt.Errorf("create log file failed: %w", err)
	}
	defer f.Close()

	gw := gzip.NewWriter(f)
	if _, err := gw.Write(cl.buf.Bytes()); err != nil {
		gw.Close()
		return fmt.Errorf("gzip write failed: %w", err)
	}
	if err := gw.Close(); err != nil {
		return fmt.Errorf("gzip close failed: %w", err)
	}

	cl.buf.Reset()
	return nil
}

func (cl *ChunkLogger) periodicFlush() {
	ticker := time.NewTicker(cl.flushInterval)
	defer ticker.Stop()

	for {
		select {
		case <-cl.stopChan:
			return
		case <-ticker.C:
			cl.Flush()
		}
	}
}
