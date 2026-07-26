package p2p

import (
	"crypto/sha256"
	"encoding/hex"
	"errors"
	"sync"
	"time"
)

// BufferedMessage represents a unique MQTT telemetry frame.
type BufferedMessage struct {
	MsgID     string    `json:"msg_id"`
	Topic     string    `json:"topic"`
	Payload   []byte    `json:"payload"`
	SeqNum    uint64    `json:"seq_num"`
	Timestamp time.Time `json:"timestamp"`
}

// MQTTBuffer manages sliding window de-duplication and message sequence buffering.
type MQTTBuffer struct {
	mu            sync.RWMutex
	capacity      int
	seenHashes    map[string]time.Time
	pendingQueue  []*BufferedMessage
	expectedSeq   uint64
	ttl           time.Duration
}

// NewMQTTBuffer creates a new MQTT deduplication & sequencing buffer.
func NewMQTTBuffer(capacity int, ttl time.Duration) *MQTTBuffer {
	if capacity <= 0 {
		capacity = 1000
	}
	if ttl <= 0 {
		ttl = 5 * time.Minute
	}
	return &MQTTBuffer{
		capacity:     capacity,
		seenHashes:   make(map[string]time.Time),
		pendingQueue: make([]*BufferedMessage, 0),
		expectedSeq:  1,
		ttl:          ttl,
	}
}

func computeHash(topic string, payload []byte, seq uint64) string {
	h := sha256.New()
	h.Write([]byte(topic))
	h.Write(payload)
	var b [8]byte
	b[0] = byte(seq >> 56)
	b[1] = byte(seq >> 48)
	b[2] = byte(seq >> 40)
	b[3] = byte(seq >> 32)
	b[4] = byte(seq >> 24)
	b[5] = byte(seq >> 16)
	b[6] = byte(seq >> 8)
	b[7] = byte(seq)
	h.Write(b[:])
	return hex.EncodeToString(h.Sum(nil))
}

// PushMessage processes an incoming MQTT message, rejecting duplicates and ordering sequence.
func (b *MQTTBuffer) PushMessage(topic string, payload []byte, seq uint64) (*BufferedMessage, bool, error) {
	if topic == "" || len(payload) == 0 {
		return nil, false, errors.New("empty topic or payload")
	}

	b.mu.Lock()
	defer b.mu.Unlock()

	b.cleanupStaleLocked()

	hash := computeHash(topic, payload, seq)
	if _, exists := b.seenHashes[hash]; exists {
		// Duplicate message detected
		return nil, true, nil
	}

	b.seenHashes[hash] = time.Now()

	msg := &BufferedMessage{
		MsgID:     hash[:16],
		Topic:     topic,
		Payload:   payload,
		SeqNum:    seq,
		Timestamp: time.Now(),
	}

	b.pendingQueue = append(b.pendingQueue, msg)

	// Trim queue if capacity exceeded
	if len(b.pendingQueue) > b.capacity {
		b.pendingQueue = b.pendingQueue[len(b.pendingQueue)-b.capacity:]
	}

	return msg, false, nil
}

// PopNextOrdered retrieves the next in-order message if available.
func (b *MQTTBuffer) PopNextOrdered() *BufferedMessage {
	b.mu.Lock()
	defer b.mu.Unlock()

	for i, msg := range b.pendingQueue {
		if msg.SeqNum == b.expectedSeq {
			b.pendingQueue = append(b.pendingQueue[:i], b.pendingQueue[i+1:]...)
			b.expectedSeq++
			return msg
		}
	}
	return nil
}

func (b *MQTTBuffer) cleanupStaleLocked() {
	now := time.Now()
	for hash, t := range b.seenHashes {
		if now.Sub(t) > b.ttl {
			delete(b.seenHashes, hash)
		}
	}
}
