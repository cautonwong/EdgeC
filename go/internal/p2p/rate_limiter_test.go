package p2p

import (
	"testing"
	"time"
)

func TestTokenBucketLimiter(t *testing.T) {
	limiter := NewTokenBucketLimiter(3.0, 10.0) // capacity 3, refill 10/sec

	// Consume initial 3 tokens
	if !limiter.Allow() || !limiter.Allow() || !limiter.Allow() {
		t.Fatalf("expected initial 3 tokens to be allowed")
	}

	// 4th token immediately should fail
	if limiter.Allow() {
		t.Fatalf("expected 4th request to be rate limited")
	}

	// Wait 200ms to refill 2 tokens (10/sec * 0.2s = 2)
	time.Sleep(200 * time.Millisecond)

	if !limiter.Allow() {
		t.Fatalf("expected request after refill to be allowed")
	}
}
