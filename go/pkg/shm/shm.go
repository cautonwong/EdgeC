package shm

import (
	"fmt"
	"os"
	"sync/atomic"
	"syscall"
	"unsafe"
)

const (
	PipeMsgSize   = 64
	ShmMagic      = 0x44484231
	ShmABIVersion = 1
	ShmHeaderSize = 32
	ShmSeqSize    = 8
)

// shmHeader mirrors datahub_shm_header_t in modules/edge_core/include/datahub_shm.h.
type shmHeader struct {
	Magic      uint32
	ABIVersion uint16
	HeaderSize uint16
	Capacity   uint32
	RecordSize uint32
	Sequence   uint64
	DataOffset uint64
}

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
	data    []PipeMsg
	seq     *uint64
	mapping []byte
	file    *os.File
	name    string
	header  *shmHeader
}

func shadowTotalSize(capacity uint32) int {
	return ShmHeaderSize + int(capacity)*PipeMsgSize
}

func attachMapped(file *os.File, mapping []byte, capacity uint32) (*ShadowMatrix, error) {
	if len(mapping) < ShmHeaderSize {
		return nil, fmt.Errorf("shm is smaller than its header")
	}
	h := (*shmHeader)(unsafe.Pointer(&mapping[0]))
	if h.Magic != ShmMagic || h.ABIVersion != ShmABIVersion ||
		h.HeaderSize != ShmHeaderSize || h.RecordSize != PipeMsgSize ||
		h.Capacity == 0 || int(h.Capacity)*PipeMsgSize > len(mapping) {
		if len(mapping)%PipeMsgSize != 0 {
			return nil, fmt.Errorf("incompatible shm header")
		}
		legacyCapacity := uint32(len(mapping) / PipeMsgSize)
		if legacyCapacity == 0 {
			return nil, fmt.Errorf("incompatible shm header")
		}
		data := unsafe.Slice((*PipeMsg)(unsafe.Pointer(&mapping[0])), int(legacyCapacity))
		return &ShadowMatrix{data: data, mapping: mapping, file: file}, nil
	}
	if capacity != 0 && h.Capacity != capacity {
		return nil, fmt.Errorf("shm capacity %d does not match requested %d", h.Capacity, capacity)
	}
	dataStart := int(h.DataOffset)
	dataEnd := dataStart + int(h.Capacity)*PipeMsgSize
	if h.DataOffset != ShmHeaderSize || dataEnd > len(mapping) {
		return nil, fmt.Errorf("shm offsets exceed mapping")
	}
	data := unsafe.Slice((*PipeMsg)(unsafe.Pointer(&mapping[dataStart])), int(h.Capacity))
	seq := &h.Sequence
	return &ShadowMatrix{
		data:    data,
		seq:     seq,
		mapping: mapping,
		file:    file,
		header:  h,
	}, nil
}

// OpenShadowMatrix maps the DataHub shadow table from shared memory
func OpenShadowMatrix(name string, capacity uint32) (*ShadowMatrix, error) {
	shmPath := fmt.Sprintf("/dev/shm/edgeos_shadow_%s", name)
	f, err := os.OpenFile(shmPath, os.O_RDWR, 0666)
	if err != nil {
		return nil, fmt.Errorf("failed to open shm: %v", err)
	}
	st, err := f.Stat()
	if err != nil {
		f.Close()
		return nil, fmt.Errorf("failed to stat shm: %v", err)
	}
	if st.Size() == 0 {
		f.Close()
		return nil, fmt.Errorf("shm is empty")
	}
	data, err := syscall.Mmap(int(f.Fd()), 0, int(st.Size()), syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_SHARED)
	if err != nil {
		f.Close()
		return nil, fmt.Errorf("failed to mmap: %v", err)
	}
	m, err := attachMapped(f, data, capacity)
	if err != nil {
		syscall.Munmap(data)
		f.Close()
		return nil, err
	}
	m.name = name
	return m, nil
}

// CreateOrOpenShadowMatrix creates or opens the DataHub shadow table from shared memory
func CreateOrOpenShadowMatrix(name string, capacity uint32) (*ShadowMatrix, error) {
	shmPath := fmt.Sprintf("/dev/shm/edgeos_shadow_%s", name)
	f, err := os.OpenFile(shmPath, os.O_CREATE|os.O_RDWR, 0666)
	if err != nil {
		return nil, fmt.Errorf("failed to create/open shm: %v", err)
	}
	if err := syscall.Flock(int(f.Fd()), syscall.LOCK_EX); err != nil {
		f.Close()
		return nil, fmt.Errorf("failed to lock shm: %v", err)
	}
	defer syscall.Flock(int(f.Fd()), syscall.LOCK_UN)

	size := shadowTotalSize(capacity)
	st, err := f.Stat()
	if err != nil {
		f.Close()
		return nil, fmt.Errorf("failed to stat shm: %v", err)
	}
	created := st.Size() == 0
	if created {
		if err := f.Truncate(int64(size)); err != nil {
			f.Close()
			return nil, fmt.Errorf("failed to truncate shm: %v", err)
		}
	}
	mapSize := size
	if !created {
		mapSize = int(st.Size())
	}
	data, err := syscall.Mmap(int(f.Fd()), 0, mapSize, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_SHARED)
	if err != nil {
		f.Close()
		return nil, fmt.Errorf("failed to mmap: %v", err)
	}
	if created {
		for i := range data {
			data[i] = 0
		}
		h := (*shmHeader)(unsafe.Pointer(&data[0]))
		h.Magic = ShmMagic
		h.ABIVersion = ShmABIVersion
		h.HeaderSize = ShmHeaderSize
		h.Capacity = capacity
		h.RecordSize = PipeMsgSize
		h.DataOffset = ShmHeaderSize
		h.Sequence = 0
	}
	m, err := attachMapped(f, data, capacity)
	if err != nil {
		syscall.Munmap(data)
		f.Close()
		return nil, err
	}
	m.name = name
	return m, nil
}

func init() {
	// [ADR-0004] C/Go zero-copy memory alignment verification
	if size := unsafe.Sizeof(PipeMsg{}); size != PipeMsgSize {
		panic(fmt.Sprintf("PipeMsg size mismatch: expected %d, got %d", PipeMsgSize, size))
	}
	if size := unsafe.Sizeof(shmHeader{}); size != ShmHeaderSize {
		panic(fmt.Sprintf("shmHeader size mismatch: expected %d, got %d", ShmHeaderSize, size))
	}
}

// ReadTag reads a message from the shadow table by index (copy return)
func (s *ShadowMatrix) ReadTag(idx uint32) (*PipeMsg, error) {
	if idx >= uint32(len(s.data)) {
		return nil, fmt.Errorf("index out of bounds: %d", idx)
	}
	if err := syscall.Flock(int(s.file.Fd()), syscall.LOCK_SH); err != nil {
		return nil, fmt.Errorf("failed to lock shm: %v", err)
	}
	defer syscall.Flock(int(s.file.Fd()), syscall.LOCK_UN)
	if s.seq == nil {
		msg := s.data[idx]
		return &msg, nil
	}
	for retry := 0; retry < 8; retry++ {
		before := atomic.LoadUint64(s.seq)
		if before&1 != 0 {
			continue
		}
		msg := s.data[idx]
		after := atomic.LoadUint64(s.seq)
		if before == after {
			return &msg, nil
		}
	}
	return nil, fmt.Errorf("shm slot %d changed while reading", idx)
}

// ReadTagDirect returns a direct pointer into the mmap shared memory (0 B/op zero-copy access).
// It is not protected by the seqlock; callers that need cross-process safety must use ReadTag.
func (s *ShadowMatrix) ReadTagDirect(idx uint32) (*PipeMsg, error) {
	if idx >= uint32(len(s.data)) {
		return nil, fmt.Errorf("index out of bounds: %d", idx)
	}
	return &s.data[idx], nil
}

// WriteTag writes a float64 value to the shadow table by index
func (s *ShadowMatrix) WriteTag(idx uint32, val float64, ts uint64) error {
	if idx >= uint32(len(s.data)) {
		return fmt.Errorf("index out of bounds: %d", idx)
	}
	if err := syscall.Flock(int(s.file.Fd()), syscall.LOCK_EX); err != nil {
		return fmt.Errorf("failed to lock shm: %v", err)
	}
	defer syscall.Flock(int(s.file.Fd()), syscall.LOCK_UN)
	if s.seq != nil {
		atomic.AddUint64(s.seq, 1)
	}
	*(*float64)(unsafe.Pointer(&s.data[idx].Payload[0])) = val
	s.data[idx].Timestamp = ts
	if s.seq != nil {
		atomic.AddUint64(s.seq, 1)
	}
	return nil
}

// Close unmaps the memory and closes the file
func (s *ShadowMatrix) Close() error {
	err := syscall.Munmap(s.mapping)
	closeErr := s.file.Close()
	if err != nil {
		return err
	}
	return closeErr
}
