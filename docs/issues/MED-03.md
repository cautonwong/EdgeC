## Parent

Reference: /workspaces/edgeos/EdgeC/MED_CTRL_PRD.md

## What to build

Implement the `hl7_fhir_exporter` protocol driver. To ensure HIPAA compliance and interoperability, raw physiological data will not be transmitted. Instead, this northbound driver will subscribe to specific clinical insights and alarm events (e.g., from the anomaly detector). It will format these state changes into standard HL7 FHIR Observation and RiskAssessment JSON objects, transmitting them securely over a mutually-authenticated TLS MQTT tunnel to the hospital's central EMR (Electronic Medical Record) system.

## Acceptance criteria

- [ ] A new `edge_driver_t` implementation for HL7 FHIR is created.
- [ ] The driver correctly serializes DataHub alarm events into valid FHIR JSON schemas.
- [ ] TLS v1.2/1.3 with client certificate authentication is enforced for the underlying MQTT transport.
- [ ] An AFK test verifies that a synthetic alarm event in the DataHub triggers the correct FHIR JSON payload generation.

## Blocked by

- [MED-02] ECG QRS Anomaly Detector Stage