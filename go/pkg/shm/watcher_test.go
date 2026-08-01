package shm

import (
	"fmt"
	"os"
	"syscall"
	"testing"
)

func TestTryOpenEventFifo(t *testing.T) {
	name := "test_fifo_unit"
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
