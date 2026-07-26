package prescription

import (
	"encoding/json"
	"testing"
)

func TestExportPrescriptionToFHIRCarePlan(t *testing.T) {
	rx := &Prescription{
		ID:          "RX-2026-TEST",
		PatientID:   "PATIENT-101",
		DoctorID:    "DOC-888",
		TargetItems: []string{"ITEM_GAIT_01", "ITEM_ROM_SHOULDER_02"},
		Intensity:   "MEDIUM",
		DurationMin: 30,
		Status:      "ACTIVE",
	}

	data, err := ExportPrescriptionToFHIRCarePlan(rx)
	if err != nil {
		t.Fatalf("Failed to export FHIR CarePlan: %v", err)
	}

	var cp FHIRCarePlan
	if err := json.Unmarshal(data, &cp); err != nil {
		t.Fatalf("Invalid JSON produced for FHIR CarePlan: %v", err)
	}

	if cp.ResourceType != "CarePlan" {
		t.Errorf("Expected ResourceType 'CarePlan', got %s", cp.ResourceType)
	}
	if cp.Subject.Reference != "Patient/PATIENT-101" {
		t.Errorf("Expected Patient reference 'Patient/PATIENT-101', got %s", cp.Subject.Reference)
	}
	if len(cp.Activity) != 2 {
		t.Errorf("Expected 2 activities, got %d", len(cp.Activity))
	}
}

func TestExportObservationToFHIRObservation(t *testing.T) {
	data, err := ExportObservationToFHIRObservation("obs-hr-001", "PATIENT-101", "8867-4", "Heart rate", 78.5, "bpm")
	if err != nil {
		t.Fatalf("Failed to export FHIR Observation: %v", err)
	}

	var obs FHIRObservation
	if err := json.Unmarshal(data, &obs); err != nil {
		t.Fatalf("Invalid JSON produced for FHIR Observation: %v", err)
	}

	if obs.ResourceType != "Observation" {
		t.Errorf("Expected ResourceType 'Observation', got %s", obs.ResourceType)
	}
	if obs.ValueQuantity.Value != 78.5 {
		t.Errorf("Expected Value 78.5, got %f", obs.ValueQuantity.Value)
	}
}
