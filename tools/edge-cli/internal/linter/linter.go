package linter

import (
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"strings"

	"gopkg.in/yaml.v3"
)

var allowedDataTypes = map[string]bool{
	"float32": true, "float64": true, "float": true, "double": true,
	"int8": true, "int16": true, "int32": true, "int64": true, "int": true,
	"uint8": true, "uint16": true, "uint32": true, "uint64": true, "uint": true,
	"bool": true, "string": true, "bytes": true, "raw": true,
}

var validIdentifier = regexp.MustCompile(`^[A-Za-z0-9_]+$`)

type Diagnostic struct {
	Level      string // ERROR, WARNING, INFO
	File       string
	Line       int
	Message    string
	Suggestion string
}

func (d Diagnostic) String() string {
	loc := d.File
	if d.Line > 0 {
		loc = fmt.Sprintf("%s:%d", d.File, d.Line)
	}
	if d.Suggestion != "" {
		return fmt.Sprintf("[%s] %s: %s (Suggestion: %s)", d.Level, loc, d.Message, d.Suggestion)
	}
	return fmt.Sprintf("[%s] %s: %s", d.Level, loc, d.Message)
}

type GenericNode struct {
	Product   string                 `yaml:"product"`
	Domain    string                 `yaml:"domain"`
	Binding   string                 `yaml:"binding"`
	Driver    string                 `yaml:"driver"`
	Protocol  string                 `yaml:"protocol"`
	Include   []string               `yaml:"include"`
	Tags      []GenericTag           `yaml:"tags"`
	Points    []GenericPoint         `yaml:"points"`
	Defaults  []yaml.Node            `yaml:"defaults"`
	Extra     map[string]interface{} `yaml:",inline"`
}

type GenericTag struct {
	ID         uint32                 `yaml:"id"`
	Name       string                 `yaml:"name"`
	Type       string                 `yaml:"type"`
	Unit       string                 `yaml:"unit"`
	Source     string                 `yaml:"source"`
	Attributes map[string]interface{} `yaml:"attributes"`
}

type GenericPoint struct {
	ID       uint32  `yaml:"id"`
	Name     string  `yaml:"name"`
	Address  string  `yaml:"address"`
	RegType  string  `yaml:"register_type"`
	Factor   float64 `yaml:"factor"`
}

type LintResult struct {
	FilePath    string
	Diagnostics []Diagnostic
	ErrorCount  int
	WarnCount   int
}

func LintContractFile(filePath string, baseDir string) (*LintResult, error) {
	data, err := os.ReadFile(filePath)
	if err != nil {
		return nil, fmt.Errorf("failed to read file: %w", err)
	}

	result := &LintResult{
		FilePath: filePath,
	}

	var root GenericNode
	if err := yaml.Unmarshal(data, &root); err != nil {
		result.Diagnostics = append(result.Diagnostics, Diagnostic{
			Level:   "ERROR",
			File:    filePath,
			Message: fmt.Sprintf("YAML syntax error: %v", err),
		})
		result.ErrorCount++
		return result, nil
	}

	// 1. Check Include Paths
	for _, inc := range root.Include {
		incPath := filepath.Join(baseDir, inc)
		if _, err := os.Stat(incPath); os.IsNotExist(err) {
			result.Diagnostics = append(result.Diagnostics, Diagnostic{
				Level:      "ERROR",
				File:       filePath,
				Message:    fmt.Sprintf("Included file does not exist: '%s'", inc),
				Suggestion: fmt.Sprintf("Verify path relative to base directory '%s'", baseDir),
			})
			result.ErrorCount++
		}
	}

	// 2. Check Tags Uniqueness & Validity
	idMap := make(map[uint32]string)
	nameMap := make(map[string]uint32)

	for _, tag := range root.Tags {
		if tag.ID == 0 {
			result.Diagnostics = append(result.Diagnostics, Diagnostic{
				Level:      "ERROR",
				File:       filePath,
				Message:    fmt.Sprintf("Tag '%s' has zero or missing ID", tag.Name),
				Suggestion: "Assign a non-zero positive integer (e.g. 1001, 2001)",
			})
			result.ErrorCount++
		} else if prevName, exists := idMap[tag.ID]; exists && prevName != "" && tag.Name != "" && prevName != tag.Name {
			result.Diagnostics = append(result.Diagnostics, Diagnostic{
				Level:      "ERROR",
				File:       filePath,
				Message:    fmt.Sprintf("Duplicate Tag ID %d assigned to '%s' (already assigned to '%s')", tag.ID, tag.Name, prevName),
				Suggestion: "Ensure each distinct tag has a unique TagID",
			})
			result.ErrorCount++
		} else {
			idMap[tag.ID] = tag.Name
		}

		if tag.Name != "" {
			if !validIdentifier.MatchString(tag.Name) {
				result.Diagnostics = append(result.Diagnostics, Diagnostic{
					Level:      "ERROR",
					File:       filePath,
					Message:    fmt.Sprintf("Invalid tag name '%s' (must be alphanumeric/underscore)", tag.Name),
					Suggestion: "Use uppercase identifier format, e.g. VOLTAGE_PHASE_A",
				})
				result.ErrorCount++
			}
			if prevID, exists := nameMap[tag.Name]; exists && prevID != tag.ID {
				result.Diagnostics = append(result.Diagnostics, Diagnostic{
					Level:      "ERROR",
					File:       filePath,
					Message:    fmt.Sprintf("Duplicate tag name '%s' with conflicting ID %d (previous was %d)", tag.Name, tag.ID, prevID),
					Suggestion: "Tag names should uniquely map to their TagID",
				})
				result.ErrorCount++
			} else {
				nameMap[tag.Name] = tag.ID
			}
		}

		if tag.Type != "" {
			cleanType := strings.ToLower(tag.Type)
			if !allowedDataTypes[cleanType] {
				result.Diagnostics = append(result.Diagnostics, Diagnostic{
					Level:      "ERROR",
					File:       filePath,
					Message:    fmt.Sprintf("Unsupported data type '%s' in tag '%s'", tag.Type, tag.Name),
					Suggestion: "Use one of: float, double, int, uint, string, bool, bytes",
				})
				result.ErrorCount++
			}
		}
	}

	// 3. Check Points in Binding contracts
	pointIDMap := make(map[uint32]string)
	for _, pt := range root.Points {
		if pt.ID == 0 {
			result.Diagnostics = append(result.Diagnostics, Diagnostic{
				Level:      "ERROR",
				File:       filePath,
				Message:    fmt.Sprintf("Point '%s' has zero or missing ID", pt.Name),
				Suggestion: "Assign a valid tag ID from schema domains",
			})
			result.ErrorCount++
		} else if prevPt, exists := pointIDMap[pt.ID]; exists && prevPt != pt.Name {
			result.Diagnostics = append(result.Diagnostics, Diagnostic{
				Level:      "WARNING",
				File:       filePath,
				Message:    fmt.Sprintf("Point ID %d mapped multiple times ('%s' and '%s')", pt.ID, pt.Name, prevPt),
				Suggestion: "Verify if multiple registers map to same TagID",
			})
			result.WarnCount++
		} else {
			pointIDMap[pt.ID] = pt.Name
		}
	}

	return result, nil
}
