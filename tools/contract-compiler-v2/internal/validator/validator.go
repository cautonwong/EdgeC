package validator

import (
	"contract-compiler-v2/internal/frontend"
	"fmt"
	"strings"
)

var AllowedDataTypes = map[string]bool{
	"float32": true, "float64": true, "float": true, "double": true,
	"int8": true, "int16": true, "int32": true, "int64": true, "int": true,
	"uint8": true, "uint16": true, "uint32": true, "uint64": true, "uint": true,
	"bool": true, "string": true, "bytes": true, "raw": true,
}

var CommonTypeAliases = map[string]string{
	"flot":             "float",
	"float_32":         "float32",
	"float_64":         "float64",
	"doubl":            "double",
	"double_precision": "double",
	"integer":          "int",
	"int_32":           "int32",
	"int_64":           "int64",
	"boolean":          "bool",
	"str":              "string",
	"text":             "string",
	"byte":             "bytes",
	"binary":           "bytes",
}

var CommonUnitAliases = map[string]string{
	"volts":    "V",
	"volt":     "V",
	"v":        "V",
	"amperes":  "A",
	"ampere":   "A",
	"amps":     "A",
	"amp":      "A",
	"a":        "A",
	"watts":    "W",
	"watt":     "W",
	"w":        "W",
	"hertz":    "Hz",
	"hz":       "Hz",
	"kilowatt": "kW",
	"kw":       "kW",
	"kwh":      "kWh",
	"kilowatthour": "kWh",
	"percent":  "%",
	"pct":      "%",
	"celsius":  "°C",
	"degc":     "°C",
}

type ValidationError struct {
	Field      string
	Message    string
	Suggestion string
}

func (e *ValidationError) Error() string {
	if e.Suggestion != "" {
		return fmt.Sprintf("Validation Error [%s]: %s (Suggestion: %s)", e.Field, e.Message, e.Suggestion)
	}
	return fmt.Sprintf("Validation Error [%s]: %s", e.Field, e.Message)
}

type Validator struct {
	Strict bool
}

func NewValidator(strict bool) *Validator {
	return &Validator{Strict: strict}
}

func (v *Validator) Validate(ast *frontend.ContractAST) []error {
	var errs []error

	// 1. Tag ID & Name Uniqueness & Type Check
	idMap := make(map[uint32]string)
	nameMap := make(map[string]uint32)

	for idx, tag := range ast.Tags {
		location := fmt.Sprintf("tags[%d]", idx)
		if tag.Name != "" {
			location = fmt.Sprintf("tag '%s'", tag.Name)
		}

		// Tag Name Check
		if tag.Name == "" {
			errs = append(errs, &ValidationError{
				Field:      location,
				Message:    "Tag name cannot be empty",
				Suggestion: "Provide a unique descriptive name for the tag",
			})
		} else if prevID, exists := nameMap[tag.Name]; exists {
			errs = append(errs, &ValidationError{
				Field:      location,
				Message:    fmt.Sprintf("Duplicate tag name '%s' (already used by tag ID %d)", tag.Name, prevID),
				Suggestion: "Rename the tag or consolidate duplicate definitions",
			})
		} else {
			nameMap[tag.Name] = tag.ID
		}

		// Tag ID Check
		if tag.ID == 0 {
			errs = append(errs, &ValidationError{
				Field:      location,
				Message:    "Tag ID cannot be zero",
				Suggestion: "Assign a positive uint32 ID within the allowed tag ID range",
			})
		} else if tag.ID < 1000 || (tag.ID > 65535 && tag.ID < 0x00010000) || tag.ID > 0x7FFFFFFF {
			errs = append(errs, &ValidationError{
				Field:      location,
				Message:    fmt.Sprintf("Tag ID %d (0x%X) is outside allowed segment ranges (Business: 1000-11999, Metrics: 12000-12999, Ext: 13000-65535, Protocol OAD: 0x00010000-0x7FFFFFFF)", tag.ID, tag.ID),
				Suggestion: suggestDomainRange(tag.Name),
			})
		} else if prevName, exists := idMap[tag.ID]; exists {
			errs = append(errs, &ValidationError{
				Field:      location,
				Message:    fmt.Sprintf("Duplicate tag ID %d (already assigned to '%s')", tag.ID, prevName),
				Suggestion: "Assign a unique tag ID",
			})
		} else {
			idMap[tag.ID] = tag.Name
		}

		// Data Type Whitelist Check
		if tag.DataType != "" {
			lowerType := strings.ToLower(tag.DataType)
			if !AllowedDataTypes[lowerType] {
				suggestion := findClosestDataType(tag.DataType)
				if suggestion == "" {
					suggestion = "Use one of the supported types: float32, float64, int32, int64, uint32, uint64, bool, string, bytes"
				} else {
					suggestion = fmt.Sprintf("Did you mean '%s'?", suggestion)
				}
				errs = append(errs, &ValidationError{
					Field:      location,
					Message:    fmt.Sprintf("Invalid data type '%s'", tag.DataType),
					Suggestion: suggestion,
				})
			}
		} else if v.Strict {
			errs = append(errs, &ValidationError{
				Field:      location,
				Message:    "Missing data type",
				Suggestion: "Explicitly set type (e.g. float32, uint32, bool)",
			})
		}

		// Unit Check in Strict mode for numeric types
		if tag.Unit != "" {
			lowerUnit := strings.ToLower(tag.Unit)
			if standardUnit, hasAlias := CommonUnitAliases[lowerUnit]; hasAlias && tag.Unit != standardUnit {
				// Notice/Suggestion for unit canonicalization
				if v.Strict && tag.Unit != standardUnit {
					errs = append(errs, &ValidationError{
						Field:      location,
						Message:    fmt.Sprintf("Non-standard unit format '%s'", tag.Unit),
						Suggestion: fmt.Sprintf("Did you mean canonical unit '%s'?", standardUnit),
					})
				}
			}
		} else if v.Strict && tag.DataType != "" && isNumericType(tag.DataType) && tag.Unit == "" {
			errs = append(errs, &ValidationError{
				Field:      location,
				Message:    fmt.Sprintf("Tag '%s' has numeric type '%s' but no unit defined", tag.Name, tag.DataType),
				Suggestion: "Specify a physical unit (e.g. V, A, Hz, W, %, °C, s, N/A)",
			})
		}
	}

	// 2. Model Blueprint Check
	modelMap := make(map[string]*frontend.ModelNode)
	for _, m := range ast.Models {
		if m.Name == "" {
			errs = append(errs, &ValidationError{
				Field:      "models",
				Message:    "Model name cannot be empty",
				Suggestion: "Provide a unique model name (e.g. Modbus_Meter_V1)",
			})
			continue
		}
		if prev, exists := modelMap[m.Name]; exists && prev != m {
			errs = append(errs, &ValidationError{
				Field:      fmt.Sprintf("model '%s'", m.Name),
				Message:    fmt.Sprintf("Duplicate model definition '%s'", m.Name),
				Suggestion: "Ensure model names are unique across includes",
			})
		}
		modelMap[m.Name] = m

		// Validate Model Points
		pointNames := make(map[string]bool)
		for _, p := range m.Points {
			if p.Name == "" {
				errs = append(errs, &ValidationError{
					Field:      fmt.Sprintf("model '%s'", m.Name),
					Message:    "Model point name cannot be empty",
					Suggestion: "Name every point in the model",
				})
			} else if pointNames[p.Name] {
				errs = append(errs, &ValidationError{
					Field:      fmt.Sprintf("model '%s'.point '%s'", m.Name, p.Name),
					Message:    fmt.Sprintf("Duplicate point name '%s' in model '%s'", p.Name, m.Name),
					Suggestion: "Ensure point names within a model are unique",
				})
			} else {
				pointNames[p.Name] = true
			}
		}
	}

	// 3. Instance Bindings Check
	for _, inst := range ast.Instances {
		location := fmt.Sprintf("instance '%s'", inst.Name)
		if inst.ModelName != "" {
			if _, exists := modelMap[inst.ModelName]; !exists {
				errs = append(errs, &ValidationError{
					Field:      location,
					Message:    fmt.Sprintf("Instance references unknown model '%s'", inst.ModelName),
					Suggestion: "Define the model in a model block or include file",
				})
			}
		}

		for _, b := range inst.Bindings {
			if b.TagID != 0 {
				if _, exists := idMap[b.TagID]; !exists {
					errs = append(errs, &ValidationError{
						Field:      fmt.Sprintf("%s.binding '%s'", location, b.Point),
						Message:    fmt.Sprintf("Binding references non-existent tag ID %d", b.TagID),
						Suggestion: "Ensure the referenced tag ID is defined in contract tags",
					})
				}
			}
		}
	}

	return errs
}

func isNumericType(dt string) bool {
	t := strings.ToLower(dt)
	return t == "float32" || t == "float64" || t == "float" || t == "double"
}

func suggestDomainRange(tagName string) string {
	upper := strings.ToUpper(tagName)
	if strings.HasPrefix(upper, "VOLTAGE_") || strings.HasPrefix(upper, "CURRENT_") || strings.HasPrefix(upper, "GRID_") || strings.HasPrefix(upper, "POWER_") {
		return "Assign Tag ID within Power Domain range 1000..1999"
	}
	if strings.HasPrefix(upper, "ENERGY_") {
		return "Assign Tag ID within Energy Domain range 3000..3999"
	}
	if strings.HasPrefix(upper, "BATTERY_") || strings.HasPrefix(upper, "BMS_") || strings.HasPrefix(upper, "SOC_") {
		return "Assign Tag ID within Storage Domain range 4000..4999"
	}
	return "Assign Tag ID within an allowed segment range (1000-11999)"
}

func findClosestDataType(input string) string {
	lower := strings.ToLower(input)
	if target, ok := CommonTypeAliases[lower]; ok {
		return target
	}

	types := []string{"float32", "float64", "float", "double", "int32", "int64", "int", "uint32", "uint64", "uint", "bool", "string", "bytes"}
	bestMatch := ""
	minDist := 999

	for _, candidate := range types {
		d := levenshtein(lower, candidate)
		if d < minDist && d <= 3 {
			minDist = d
			bestMatch = candidate
		}
	}
	return bestMatch
}

func levenshtein(s1, s2 string) int {
	r1, r2 := []rune(s1), []rune(s2)
	l1, l2 := len(r1), len(r2)

	col := make([]int, l1+1)
	for y := 0; y <= l1; y++ {
		col[y] = y
	}

	for x := 1; x <= l2; x++ {
		col[0] = x
		lastDiag := x - 1
		for y := 1; y <= l1; y++ {
			oldDiag := col[y]
			cost := 0
			if r1[y-1] != r2[x-1] {
				cost = 1
			}
			col[y] = min3(col[y]+1, col[y-1]+1, lastDiag+cost)
			lastDiag = oldDiag
		}
	}
	return col[l1]
}

func min3(a, b, c int) int {
	if a < b {
		if a < c {
			return a
		}
		return c
	}
	if b < c {
		return b
	}
	return c
}
