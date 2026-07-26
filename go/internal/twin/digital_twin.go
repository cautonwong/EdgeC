package twin

import (
	"encoding/json"
	"fmt"
	"sync"
	"time"
)

type FeatureState map[string]interface{}

type ThingTwin struct {
	ThingID        string                  `json:"thing_id"`
	PolicyID       string                  `json:"policy_id"`
	Reported       map[string]FeatureState `json:"reported"`
	Desired        map[string]FeatureState `json:"desired"`
	Delta          map[string]FeatureState `json:"delta,omitempty"`
	LastModified   time.Time               `json:"last_modified"`
	Version        int64                   `json:"version"`
	mu             sync.RWMutex
}

func NewThingTwin(thingID string) *ThingTwin {
	return &ThingTwin{
		ThingID:      thingID,
		PolicyID:     "smartbox:policy:v1",
		Reported:     make(map[string]FeatureState),
		Desired:      make(map[string]FeatureState),
		Delta:        make(map[string]FeatureState),
		LastModified: time.Now(),
		Version:      1,
	}
}

func (t *ThingTwin) UpdateReported(feature string, properties map[string]interface{}) {
	t.mu.Lock()
	defer t.mu.Unlock()

	if _, ok := t.Reported[feature]; !ok {
		t.Reported[feature] = make(FeatureState)
	}
	for k, v := range properties {
		t.Reported[feature][k] = v
	}
	t.recomputeDeltaLocked()
	t.Version++
	t.LastModified = time.Now()
}

func (t *ThingTwin) UpdateDesired(feature string, properties map[string]interface{}) {
	t.mu.Lock()
	defer t.mu.Unlock()

	if _, ok := t.Desired[feature]; !ok {
		t.Desired[feature] = make(FeatureState)
	}
	for k, v := range properties {
		t.Desired[feature][k] = v
	}
	t.recomputeDeltaLocked()
	t.Version++
	t.LastModified = time.Now()
}

func (t *ThingTwin) recomputeDeltaLocked() {
	t.Delta = make(map[string]FeatureState)

	for feature, desiredProps := range t.Desired {
		reportedProps, exists := t.Reported[feature]
		if !exists {
			t.Delta[feature] = copyFeatureState(desiredProps)
			continue
		}

		diff := make(FeatureState)
		for k, dVal := range desiredProps {
			rVal, rExists := reportedProps[k]
			if !rExists || fmt.Sprintf("%v", dVal) != fmt.Sprintf("%v", rVal) {
				diff[k] = dVal
			}
		}

		if len(diff) > 0 {
			t.Delta[feature] = diff
		}
	}
}

func (t *ThingTwin) SnapshotJSON() ([]byte, error) {
	t.mu.RLock()
	defer t.mu.RUnlock()
	return json.Marshal(t)
}

func copyFeatureState(fs FeatureState) FeatureState {
	copied := make(FeatureState)
	for k, v := range fs {
		copied[k] = v
	}
	return copied
}
