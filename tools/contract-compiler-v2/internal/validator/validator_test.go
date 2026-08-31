package validator

import (
	"contract-compiler-v2/internal/frontend"
	"strings"
	"testing"
)

func TestValidator(t *testing.T) {
	tests := []struct {
		name           string
		strict         bool
		ast            *frontend.ContractAST
		expectedErrors int
		errorSubstr    string
	}{
		{
			name:   "Valid Contract",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "VOLTAGE_PHASE_A", DataType: "float32", Unit: "V"},
					{ID: 1002, Name: "CURRENT_PHASE_A", DataType: "float32", Unit: "A"},
				},
			},
			expectedErrors: 0,
		},
		{
			name:   "Duplicate Tag ID",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "VOLTAGE_PHASE_A", DataType: "float32"},
					{ID: 1001, Name: "DUPLICATE_VOLTAGE", DataType: "float32"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Duplicate tag ID 1001",
		},
		{
			name:   "Duplicate Tag Name",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "VOLTAGE_PHASE_A", DataType: "float32"},
					{ID: 1002, Name: "VOLTAGE_PHASE_A", DataType: "float32"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Duplicate tag name 'VOLTAGE_PHASE_A'",
		},
		{
			name:   "Empty Tag Name",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "", DataType: "float32"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Tag name cannot be empty",
		},
		{
			name:   "Zero Tag ID",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 0, Name: "INVALID_ZERO_TAG", DataType: "float32"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Tag ID cannot be zero",
		},
		{
			name:   "Invalid Data Type with Typo Suggestion",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "BAD_TYPE_TAG", DataType: "flot"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Did you mean 'float'?",
		},
		{
			name:   "Tag ID Out of Range with Domain Suggestion",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 50, Name: "VOLTAGE_LOW_ID", DataType: "float32"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Assign Tag ID within Power Domain range 1000..1999",
		},
		{
			name:   "Strict Mode Non-Standard Unit Suggestion",
			strict: true,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "VOLTAGE_NON_STD", DataType: "float32", Unit: "Volts"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Did you mean canonical unit 'V'?",
		},
		{
			name:   "Strict Mode Missing Unit",
			strict: true,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "VOLTAGE_NO_UNIT", DataType: "float32", Unit: ""},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "no unit defined",
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			v := NewValidator(tt.strict)
			errs := v.Validate(tt.ast)

			if len(errs) != tt.expectedErrors {
				t.Fatalf("Expected %d errors, got %d: %v", tt.expectedErrors, len(errs), errs)
			}

			if tt.errorSubstr != "" && len(errs) > 0 {
				found := false
				for _, err := range errs {
					if strings.Contains(err.Error(), tt.errorSubstr) {
						found = true
						break
					}
				}
				if !found {
					t.Errorf("Expected error containing '%s', got: %v", tt.errorSubstr, errs)
				}
			}
		})
	}
}
