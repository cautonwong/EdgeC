package p2p

import (
	"testing"
	"time"
)

func TestMQTTBufferDeduplicationAndOrdering(t *testing.T) {
	buf := NewMQTTBuffer(100, 1*time.Minute)

	// 1. Push message 1
	msg1, dup, err := buf.PushMessage("telemetry/v1", []byte("data_1"), 1)
	if err != nil || dup || msg1 == nil {
		t.Fatalf("failed to push msg1: err=%v dup=%v", err, dup)
	}

	// 2. Push duplicate message 1
	_, dup2, err := buf.PushMessage("telemetry/v1", []byte("data_1"), 1)
	if err != nil || !dup2 {
		t.Fatalf("expected duplicate for msg1, got dup=%v", dup2)
	}

	// 3. Push out-of-order message 3 then message 2
	_, dup3, _ := buf.PushMessage("telemetry/v1", []byte("data_3"), 3)
	if dup3 {
		t.Fatalf("msg3 should not be duplicate")
	}

	_, dup2_2, _ := buf.PushMessage("telemetry/v1", []byte("data_2"), 2)
	if dup2_2 {
		t.Fatalf("msg2 should not be duplicate")
	}

	// 4. Pop in order: expect 1, then 2, then 3
	out1 := buf.PopNextOrdered()
	if out1 == nil || out1.SeqNum != 1 {
		t.Fatalf("expected seq 1, got %v", out1)
	}

	out2 := buf.PopNextOrdered()
	if out2 == nil || out2.SeqNum != 2 {
		t.Fatalf("expected seq 2, got %v", out2)
	}

	out3 := buf.PopNextOrdered()
	if out3 == nil || out3.SeqNum != 3 {
		t.Fatalf("expected seq 3, got %v", out3)
	}

	out4 := buf.PopNextOrdered()
	if out4 != nil {
		t.Fatalf("expected nil for empty queue pop, got %v", out4)
	}
}
