package shm

import (
	"fmt"
	"os"
	"syscall"
	"testing"
	"unsafe"
)

func TestTryOpenEventFifo(t *testing.T) {
	name := fmt.Sprintf("test_fifo_unit_%d", os.Getpid())
	fifoPath := fmt.Sprintf("/tmp/edgeos_ev_%s", name)
	_ = os.Remove(fifoPath)

	err := syscall.Mkfifo(fifoPath, 0666)
	if err != nil {
		t.Fatalf("failed to create fifo: %v", err)
	}
	defer os.Remove(fifoPath)

	// Simulate C's shmring_create opening FIFO in O_RDWR mode
	writer, err := os.OpenFile(fifoPath, os.O_RDWR, 0666)
	if err != nil {
		t.Fatalf("failed to open dummy writer: %v", err)
	}
	defer writer.Close()

	epollFd, fifoFile, err := tryOpenEventFifo(name)
	if err != nil {
		t.Fatalf("tryOpenEventFifo failed: %v", err)
	}
	defer syscall.Close(epollFd)
	defer fifoFile.Close()
}

func TestZeroCopyReadDirect(t *testing.T) {
	msg := &PipeMsg{
		Timestamp: 12345678,
		TagID:     1001,
		Type:      1,
	}
	*(*float64)(unsafe.Pointer(&msg.Payload[0])) = 220.5

	f, _ := msg.Scalar()
	if f != 220.5 {
		t.Fatalf("expected 220.5, got %v", f)
	}
}

func BenchmarkReadTagDirect(b *testing.B) {
	sm := &ShadowMatrix{
		data: make([]PipeMsg, 1024),
	}
	sm.WriteTag(5, 311.0, 99999)

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		msg, _ := sm.ReadTagDirect(5)
		_ = msg.Timestamp
	}
}
