package shm

import (
	"context"
	"fmt"
	"os"
	"syscall"
	"time"
)

// Watch changes on specific tags and push to channel.
// Uses FIFO + epoll for O(1) wakeup when available;
// falls back to 10ms polling when unavailable.
func (s *ShadowMatrix) Watch(ctx context.Context, tagIDs []uint32, ch chan<- *PipeMsg) {
	// Try FIFO-based epoll first
	epollFd, fifoFile, err := tryOpenEventFifo(s.name)
	if err == nil {
		s.watchWithEpoll(ctx, epollFd, fifoFile, tagIDs, ch)
		return
	}

	// Fallback: 100Hz polling
	s.watchWithPolling(ctx, tagIDs, ch)
}

// tryOpenEventFifo opens the FIFO created by C's shmring_create and sets up epoll.
func tryOpenEventFifo(name string) (int, *os.File, error) {
	fifoPath := fmt.Sprintf("/tmp/edgeos_ev_%s", name)
	f, err := os.OpenFile(fifoPath, os.O_RDONLY, os.ModeNamedPipe)
	if err != nil {
		return -1, nil, err
	}

	epfd, err := syscall.EpollCreate1(0)
	if err != nil {
		f.Close()
		return -1, nil, err
	}

	var event = &syscall.EpollEvent{
		Events: syscall.EPOLLIN,
		Fd:     int32(f.Fd()),
	}
	if err := syscall.EpollCtl(epfd, syscall.EPOLL_CTL_ADD, int(f.Fd()), event); err != nil {
		syscall.Close(epfd)
		f.Close()
		return -1, nil, err
	}

	return epfd, f, nil
}

// watchWithEpoll blocks on epoll for O(1) wakeup from C's shm_ring_write.
func (s *ShadowMatrix) watchWithEpoll(ctx context.Context, epollFd int, fifoFile *os.File, tagIDs []uint32, ch chan<- *PipeMsg) {
	defer syscall.Close(epollFd)
	defer fifoFile.Close()

	lastTs := make(map[uint32]uint64)
	epollEvents := make([]syscall.EpollEvent, 4)
	buf := make([]byte, 8)

	for {
		select {
		case <-ctx.Done():
			return
		default:
		}

		nevents, err := syscall.EpollWait(epollFd, epollEvents, 100)
		if err != nil {
			if err == syscall.EINTR {
				continue
			}
			// Fallback to polling on persistent epoll error
			s.watchWithPolling(ctx, tagIDs, ch)
			return
		}

		if nevents > 0 {
			// Drain the FIFO to clear the event (non-blocking)
			for {
				_, err := fifoFile.Read(buf)
				if err != nil {
					break
				}
			}

			// Read updated tags from SHM
			for _, tagID := range tagIDs {
				msg, err := s.ReadTag(tagID)
				if err == nil {
					if last, ok := lastTs[tagID]; !ok || last != msg.Timestamp {
						lastTs[tagID] = msg.Timestamp
						pushMsg := *msg
						select {
						case ch <- &pushMsg:
						default:
						}
					}
				}
			}
		}
	}
}

// watchWithPolling is the original 100Hz polling fallback
func (s *ShadowMatrix) watchWithPolling(ctx context.Context, tagIDs []uint32, ch chan<- *PipeMsg) {
	ticker := time.NewTicker(10 * time.Millisecond)
	defer ticker.Stop()

	lastTs := make(map[uint32]uint64)

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			for _, tagID := range tagIDs {
				msg, err := s.ReadTag(tagID)
				if err == nil {
					if last, ok := lastTs[tagID]; !ok || last != msg.Timestamp {
						lastTs[tagID] = msg.Timestamp
						pushMsg := *msg
						select {
						case ch <- &pushMsg:
						default:
						}
					}
				}
			}
		}
	}
}
