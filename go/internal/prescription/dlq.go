package prescription

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sync"
	"time"
)

type FailedSyncRecord struct {
	ID        string      `json:"id"`
	Payload   WSMessage   `json:"payload"`
	Error     string      `json:"error"`
	Retries   int         `json:"retries"`
	NextRetry time.Time   `json:"next_retry"`
	CreatedAt time.Time   `json:"created_at"`
}

type DeadLetterQueue struct {
	mu        sync.Mutex
	records   map[string]*FailedSyncRecord
	dlqFile   string
	maxRetry  int
}

func NewDeadLetterQueue(storageDir string) (*DeadLetterQueue, error) {
	if err := os.MkdirAll(storageDir, 0755); err != nil {
		return nil, fmt.Errorf("mkdir dlq dir failed: %w", err)
	}

	filePath := filepath.Join(storageDir, "prescription_dlq.json")
	dlq := &DeadLetterQueue{
		records:  make(map[string]*FailedSyncRecord),
		dlqFile:  filePath,
		maxRetry: 5,
	}

	dlq.loadFromDisk()
	return dlq, nil
}

func (dlq *DeadLetterQueue) Enqueue(payload WSMessage, syncErr error) {
	dlq.mu.Lock()
	defer dlq.mu.Unlock()

	id := fmt.Sprintf("dlq_%d_%d", payload.Timestamp, time.Now().UnixNano()%10000)
	record := &FailedSyncRecord{
		ID:        id,
		Payload:   payload,
		Error:     syncErr.Error(),
		Retries:   0,
		NextRetry: time.Now().Add(2 * time.Second),
		CreatedAt: time.Now(),
	}

	dlq.records[id] = record
	dlq.saveToDiskLocked()
}

func (dlq *DeadLetterQueue) ProcessPending(retryFn func(msg WSMessage) error) (int, int) {
	dlq.mu.Lock()
	defer dlq.mu.Unlock()

	now := time.Now()
	successCount := 0
	droppedCount := 0

	for id, rec := range dlq.records {
		if now.Before(rec.NextRetry) {
			continue
		}

		err := retryFn(rec.Payload)
		if err == nil {
			delete(dlq.records, id)
			successCount++
		} else {
			rec.Retries++
			rec.Error = err.Error()
			if rec.Retries >= dlq.maxRetry {
				delete(dlq.records, id)
				droppedCount++
			} else {
				// Exponential backoff
				backoffSec := 1 << rec.Retries // 2s, 4s, 8s, 16s, 32s
				rec.NextRetry = now.Add(time.Duration(backoffSec) * time.Second)
			}
		}
	}

	if successCount > 0 || droppedCount > 0 {
		dlq.saveToDiskLocked()
	}

	return successCount, droppedCount
}

func (dlq *DeadLetterQueue) PendingCount() int {
	dlq.mu.Lock()
	defer dlq.mu.Unlock()
	return len(dlq.records)
}

func (dlq *DeadLetterQueue) loadFromDisk() {
	data, err := os.ReadFile(dlq.dlqFile)
	if err != nil {
		return
	}
	var loaded map[string]*FailedSyncRecord
	if err := json.Unmarshal(data, &loaded); err == nil && loaded != nil {
		dlq.records = loaded
	}
}

func (dlq *DeadLetterQueue) saveToDiskLocked() {
	data, err := json.MarshalIndent(dlq.records, "", "  ")
	if err != nil {
		return
	}
	_ = os.WriteFile(dlq.dlqFile, data, 0644)
}
