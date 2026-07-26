package prescription

import (
	"encoding/json"
	"fmt"
	"time"
)

// FHIRObservation represents an HL7 FHIR R4 Observation resource.
type FHIRObservation struct {
	ResourceType string `json:"resourceType"` // "Observation"
	ID           string `json:"id"`
	Status       string `json:"status"` // "final"
	Code         struct {
		Coding []struct {
			System  string `json:"system"`
			Code    string `json:"code"`
			Display string `json:"display"`
		} `json:"coding"`
		Text string `json:"text"`
	} `json:"code"`
	Subject struct {
		Reference string `json:"reference"`
	} `json:"subject"`
	EffectiveDateTime string `json:"effectiveDateTime"`
	ValueQuantity     *struct {
		Value  float64 `json:"value"`
		Unit   string  `json:"unit"`
		System string  `json:"system"`
		Code   string  `json:"code"`
	} `json:"valueQuantity,omitempty"`
}

// FHIRCarePlan represents an HL7 FHIR R4 CarePlan resource mapping a rehabilitation prescription.
type FHIRCarePlan struct {
	ResourceType string `json:"resourceType"` // "CarePlan"
	ID           string `json:"id"`
	Status       string `json:"status"` // "active"
	Intent       string `json:"intent"` // "plan"
	Subject      struct {
		Reference string `json:"reference"`
	} `json:"subject"`
	Author struct {
		Reference string `json:"reference"`
	} `json:"author"`
	Activity []struct {
		Detail struct {
			Code struct {
				Text string `json:"text"`
			} `json:"code"`
			Status string `json:"status"`
		} `json:"detail"`
	} `json:"activity"`
}

// ExportPrescriptionToFHIRCarePlan converts a Rehabilitation Prescription to an HL7 FHIR CarePlan JSON resource.
func ExportPrescriptionToFHIRCarePlan(p *Prescription) ([]byte, error) {
	if p == nil {
		return nil, fmt.Errorf("nil prescription")
	}
	cp := FHIRCarePlan{
		ResourceType: "CarePlan",
		ID:           fmt.Sprintf("fhir-cp-%s", p.ID),
		Status:       "active",
		Intent:       "plan",
	}
	cp.Subject.Reference = fmt.Sprintf("Patient/%s", p.PatientID)
	cp.Author.Reference = fmt.Sprintf("Practitioner/%s", p.DoctorID)

	for _, item := range p.TargetItems {
		act := struct {
			Detail struct {
				Code struct {
					Text string `json:"text"`
				} `json:"code"`
				Status string `json:"status"`
			} `json:"detail"`
		}{}
		act.Detail.Code.Text = item
		act.Detail.Status = "in-progress"
		cp.Activity = append(cp.Activity, act)
	}

	return json.MarshalIndent(cp, "", "  ")
}

// ExportObservationToFHIRObservation converts a telemetry reading (e.g. Heart Rate, ROM Angle) to an HL7 FHIR Observation JSON resource.
func ExportObservationToFHIRObservation(id string, patientID string, codeStr string, display string, val float64, unit string) ([]byte, error) {
	obs := FHIRObservation{
		ResourceType:      "Observation",
		ID:                id,
		Status:            "final",
		EffectiveDateTime: time.Now().Format(time.RFC3339),
	}
	obs.Subject.Reference = fmt.Sprintf("Patient/%s", patientID)
	obs.Code.Text = display
	obs.Code.Coding = []struct {
		System  string `json:"system"`
		Code    string `json:"code"`
		Display string `json:"display"`
	}{
		{
			System:  "http://loinc.org",
			Code:    codeStr,
			Display: display,
		},
	}
	obs.ValueQuantity = &struct {
		Value  float64 `json:"value"`
		Unit   string  `json:"unit"`
		System string  `json:"system"`
		Code   string  `json:"code"`
	}{
		Value:  val,
		Unit:   unit,
		System: "http://unitsofmeasure.org",
		Code:   unit,
	}

	return json.MarshalIndent(obs, "", "  ")
}
