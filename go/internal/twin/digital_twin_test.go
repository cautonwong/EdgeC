package twin

import (
	"encoding/json"
	"testing"
)

func TestThingTwinDeltaReconciliation(t *testing.T) {
	twin := NewThingTwin("org.edgeos:smartbox-alpha-01")

	// 1. Set reported state (physical battery 85%, prescription 1001)
	twin.UpdateReported("telemetry", map[string]interface{}{
		"battery_soc":           85.0,
		"rehab_prescription_id": 1001,
	})

	// 2. Set desired state from cloud (prescription 1002, volume 80)
	twin.UpdateDesired("telemetry", map[string]interface{}{
		"battery_soc":           85.0,
		"rehab_prescription_id": 1002, // Delta!
		"speaker_volume":        80,   // Delta!
	})

	// 3. Verify delta calculation
	twin.mu.RLock()
	delta, exists := twin.Delta["telemetry"]
	twin.mu.RUnlock()

	if !exists {
		t.Fatal("Expected telemetry delta feature to exist")
	}

	if _, ok := delta["battery_soc"]; ok {
		t.Error("battery_soc is identical in reported and desired, should not be in delta")
	}

	if val, ok := delta["rehab_prescription_id"]; !ok || val != 1002 {
		t.Errorf("Expected rehab_prescription_id=1002 in delta, got %v", val)
	}

	// 4. Verify JSON output
	data, err := twin.SnapshotJSON()
	if err != nil {
		t.Fatalf("Failed to generate twin JSON snapshot: %v", err)
	}

	var parsed map[string]interface{}
	if err := json.Unmarshal(data, &parsed); err != nil {
		t.Fatalf("Invalid JSON output: %v", err)
	}

	t.Logf("Digital Twin Snapshot JSON: %s", string(data))
}
