package validator

import (
	"contract-compiler-v2/internal/frontend"
	"strings"
	"testing"
)

func TestValidator(t *testing.T) {
	tests := []struct {
		name          string
		strict        bool
		ast           *frontend.ContractAST
		expectedErrors int
		errorSubstr   string
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
			name:   "Invalid Data Type",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "BAD_TYPE_TAG", DataType: "invalid_custom_struct"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Invalid data type 'invalid_custom_struct'",
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
		{
			name:   "Strict Mode Missing Data Type",
			strict: true,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "NO_TYPE_TAG", DataType: ""},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Missing data type",
		},
		{
			name:   "Instance Unknown Model Reference",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "TAG_1001", DataType: "uint32"},
				},
				Instances: []*frontend.InstanceNode{
					{Name: "Meter1", ModelName: "NonExistentModel"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "references unknown model 'NonExistentModel'",
		},
		{
			name:   "Instance Binding Unknown TagID",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 1001, Name: "TAG_1001", DataType: "uint32"},
				},
				Models: []*frontend.ModelNode{
					{Name: "MeterModel", Points: []*frontend.PointNode{{Name: "P1"}}},
				},
				Instances: []*frontend.InstanceNode{
					{
						Name:      "Meter1",
						ModelName: "MeterModel",
						Bindings: []*frontend.BindingMapNode{
							{Point: "P1", TagID: 99999}, // 99999 does not exist
						},
					},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "references non-existent tag ID 99999",
		},
		{
			name:   "Duplicate Model Point Name",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Models: []*frontend.ModelNode{
					{
						Name: "BadModel",
						Points: []*frontend.PointNode{
							{Name: "POINT_A", Type: "float32"},
							{Name: "POINT_A", Type: "uint32"},
						},
					},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "Duplicate point name 'POINT_A'",
		},
		{
			name:   "Tag ID Out of Segment Range",
			strict: false,
			ast: &frontend.ContractAST{
				ProductName: "SmartBox",
				Tags: []*frontend.TagNode{
					{ID: 500, Name: "INVALID_LOW_TAG", DataType: "float32"},
				},
			},
			expectedErrors: 1,
			errorSubstr:    "outside allowed segment ranges",
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
