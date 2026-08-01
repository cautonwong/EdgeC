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

	epollFd, fifoFile, err := tryOpenEventFifo(name)
	if err != nil {
		t.Fatalf("tryOpenEventFifo failed: %v", err)
	}
	defer syscall.Close(epollFd)
	defer fifoFile.Close()
}
