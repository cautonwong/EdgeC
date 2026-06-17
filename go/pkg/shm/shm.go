package shm

import (
	"fmt"
	"os"
	"reflect"
	"syscall"
	"unsafe"
)

const (
	PipeMsgSize = 64
)

// PipeMsg mirrors the C structure
type PipeMsg struct {
	Timestamp  uint64   // 0..7
	TagID      uint32   // 8..11
	DeviceID   uint32   // 12..15
	Type       uint16   // 16..17
	State      uint16   // 18..19
	DropReason uint32   // 20..23
	Quality    uint8    // 24
	Priority   uint8    // 25
	SourceID   uint8    // 26
	Reserved   [5]byte  // 27..31
	Payload    [32]byte // 32..63
}

// Scalar returns the float64 and int64 values from the payload
func (m *PipeMsg) Scalar() (float64, int64) {
	f := *(*float64)(unsafe.Pointer(&m.Payload[0]))
	i := *(*int64)(unsafe.Pointer(&m.Payload[8]))
	return f, i
}

// ShadowMatrix represents the mapped shadow table
type ShadowMatrix struct {
	data []PipeMsg
	file *os.File
}

// OpenShadowMatrix maps the DataHub shadow table from shared memory
func OpenShadowMatrix(name string, capacity uint32) (*ShadowMatrix, error) {
	shmPath := fmt.Sprintf("/dev/shm/edgeos_shadow_%s", name)
	f, err := os.OpenFile(shmPath, os.O_RDWR, 0666)
	if err != nil {
		return nil, fmt.Errorf("failed to open shm: %v", err)
	}

	size := int(capacity) * PipeMsgSize
	data, err := syscall.Mmap(int(f.Fd()), 0, size, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_SHARED)
	if err != nil {
		f.Close()
		return nil, fmt.Errorf("failed to mmap: %v", err)
	}

	// Slice trick to treat mmapped memory as a slice of PipeMsg
	header := (*reflect.SliceHeader)(unsafe.Pointer(&data))
	header.Len = int(capacity)
	header.Cap = int(capacity)

	return &ShadowMatrix{
		data: *(*[]PipeMsg)(unsafe.Pointer(header)),
		file: f,
	}, nil
}

// ReadTag reads a message from the shadow table by index
func (s *ShadowMatrix) ReadTag(idx uint32) (*PipeMsg, error) {
	if idx >= uint32(len(s.data)) {
		return nil, fmt.Errorf("index out of bounds: %d", idx)
	}
	// Return a copy to ensure safety
	msg := s.data[idx]
	return &msg, nil
}

// Close unmaps the memory and closes the file
func (s *ShadowMatrix) Close() error {
	header := (*reflect.SliceHeader)(unsafe.Pointer(&s.data))
	data := *(*[]byte)(unsafe.Pointer(header))
	data = data[:cap(data)] // Ensure we unmap the full capacity

	if err := syscall.Munmap(data); err != nil {
		s.file.Close()
		return err
	}
	return s.file.Close()
}
