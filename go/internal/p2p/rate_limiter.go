package p2p

import (
	"sync"
	"time"
)

// TokenBucketLimiter implements thread-safe token bucket rate limiting for edge APIs.
type TokenBucketLimiter struct {
	mu           sync.Mutex
	capacity     float64
	tokens       float64
	refillRate   float64 // tokens per second
	lastRefillAt time.Time
}

// NewTokenBucketLimiter creates a token bucket limiter with given capacity and refill rate per second.
func NewTokenBucketLimiter(capacity float64, refillRate float64) *TokenBucketLimiter {
	if capacity <= 0 {
		capacity = 100
	}
	if refillRate <= 0 {
		refillRate = 10
	}
	return &TokenBucketLimiter{
		capacity:     capacity,
		tokens:       capacity,
		refillRate:   refillRate,
		lastRefillAt: time.Now(),
	}
}

// Allow checks if 1 token can be consumed immediately.
func (l *TokenBucketLimiter) Allow() bool {
	return l.AllowN(1.0)
}

// AllowN checks if N tokens can be consumed immediately.
func (l *TokenBucketLimiter) AllowN(n float64) bool {
	l.mu.Lock()
	defer l.mu.Unlock()

	now := time.Now()
	elapsed := now.Sub(l.lastRefillAt).Seconds()
	l.lastRefillAt = now

	// Refill tokens
	l.tokens += elapsed * l.refillRate
	if l.tokens > l.capacity {
		l.tokens = l.capacity
	}

	if l.tokens >= n {
		l.tokens -= n
		return true
	}

	return false
}
