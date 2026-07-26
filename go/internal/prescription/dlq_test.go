package prescription

import (
	"errors"
	"os"
	"testing"
	"time"
)

func TestDeadLetterQueueRetryBackoff(t *testing.T) {
	tempDir, err := os.MkdirTemp("", "dlq_test")
	if err != nil {
		t.Fatalf("Failed to create temp dir: %v", err)
	}
	defer os.RemoveAll(tempDir)

	dlq, err := NewDeadLetterQueue(tempDir)
	if err != nil {
		t.Fatalf("Failed to create DLQ: %v", err)
	}

	msg := WSMessage{
		Action:    "sync_prescription",
		Timestamp: time.Now().Unix(),
	}

	// 1. Enqueue failed message
	dlq.Enqueue(msg, errors.New("network timeout connection refused"))
	if count := dlq.PendingCount(); count != 1 {
		t.Fatalf("Expected 1 pending record, got %d", count)
	}

	// 2. Retry before NextRetry should skip
	success, dropped := dlq.ProcessPending(func(payload WSMessage) error {
		return nil
	})
	if success != 0 || dropped != 0 {
		t.Fatalf("Expected 0 processed before NextRetry, got success=%d dropped=%d", success, dropped)
	}

	// 3. Fast-forward record NextRetry
	dlq.mu.Lock()
	for _, rec := range dlq.records {
		rec.NextRetry = time.Now().Add(-1 * time.Second)
	}
	dlq.mu.Unlock()

	// 4. Retry with success should resolve and clear record
	success, dropped = dlq.ProcessPending(func(payload WSMessage) error {
		return nil
	})
	if success != 1 {
		t.Fatalf("Expected 1 successful retry, got %d", success)
	}

	if count := dlq.PendingCount(); count != 0 {
		t.Fatalf("Expected 0 pending records after success, got %d", count)
	}
}
