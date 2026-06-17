package shm

import (
	"context"
	"time"
)

// Watch changes on specific tags and push to channel
func (s *ShadowMatrix) Watch(ctx context.Context, tagIDs []uint32, ch chan<- *PipeMsg) {
	// A simple polling watcher for the mapped shadow table.
	// In a real industrial scenario, this would block on a semaphore or eventfd
	// signaled by the C core when the ring buffer updates.
	// For this demo, we poll the memory at 100Hz.
	
	ticker := time.NewTicker(10 * time.Millisecond)
	defer ticker.Stop()

	// Keep track of timestamps to only send updates
	lastTs := make(map[uint32]uint64)

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			for _, tagID := range tagIDs {
				msg, err := s.ReadTag(tagID) // Logical index maps 1:1 with tagID for demo
				if err == nil {
					if last, ok := lastTs[tagID]; !ok || last != msg.Timestamp {
						lastTs[tagID] = msg.Timestamp
						// Copy message to send
						pushMsg := *msg
						select {
						case ch <- &pushMsg:
						default:
							// Client channel full, drop to avoid blocking watcher
						}
					}
				}
			}
		}
	}
}
