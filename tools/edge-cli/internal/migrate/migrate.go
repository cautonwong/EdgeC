package migrate

import (
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"regexp"
	"strings"
)

type MigrateOptions struct {
	TargetDir string
	Fix       bool
	Verbose   bool
}

type MigrationIssue struct {
	File        string
	LineNumber  int
	Description string
	OldText     string
	NewText     string
}

type MigrateResult struct {
	ScannedFiles  int
	IssuesFound   int
	IssuesFixed   int
	ModifiedFiles []string
	Issues        []MigrationIssue
}

var (
	// Legacy include path replacements
	cHeaderRules = []struct {
		Pattern     *regexp.Regexp
		Replacement string
		Desc        string
	}{
		{
			Pattern:     regexp.MustCompile(`#include\s+["<]edgeos\.h[">]`),
			Replacement: `#include <edgeos/edgeos.h>`,
			Desc:        "Migrate legacy edgeos.h to unified SDK public header <edgeos/edgeos.h>",
		},
		{
			Pattern:     regexp.MustCompile(`#include\s+["<]edge_driver\.h[">]`),
			Replacement: `#include <edgeos/driver.h>`,
			Desc:        "Migrate direct edge_driver.h to SDK public header <edgeos/driver.h>",
		},
		{
			Pattern:     regexp.MustCompile(`#include\s+["<]datahub\.h[">]`),
			Replacement: `#include <edgeos/datahub.h>`,
			Desc:        "Migrate direct datahub.h to SDK public header <edgeos/datahub.h>",
		},
		{
			Pattern:     regexp.MustCompile(`#include\s+["<]edge_anti_corruption\.h[">]`),
			Replacement: `#include <edgeos/anti_corruption.h>`,
			Desc:        "Migrate direct edge_anti_corruption.h to SDK header <edgeos/anti_corruption.h>",
		},
	}

	// YAML contract modernization rules
	yamlRules = []struct {
		Pattern     *regexp.Regexp
		Replacement string
		Desc        string
	}{
		{
			Pattern:     regexp.MustCompile(`(?m)^(\s*)include:\s*["']?([^"'\n\r]+)["']?`),
			Replacement: "${1}includes:\n${1}  - \"$2\"",
			Desc:        "Modernize singular 'include:' to standard list 'includes:'",
		},
		{
			Pattern:     regexp.MustCompile(`(?i)type:\s*["']?float_32["']?`),
			Replacement: `type: "float32"`,
			Desc:        "Normalize legacy data type 'float_32' to 'float32'",
		},
		{
			Pattern:     regexp.MustCompile(`(?i)type:\s*["']?float_64["']?`),
			Replacement: `type: "float64"`,
			Desc:        "Normalize legacy data type 'float_64' to 'float64'",
		},
		{
			Pattern:     regexp.MustCompile(`(?i)unit:\s*["']?Volts["']?`),
			Replacement: `unit: "V"`,
			Desc:        "Normalize unit 'Volts' to canonical standard 'V'",
		},
		{
			Pattern:     regexp.MustCompile(`(?i)unit:\s*["']?Amperes["']?`),
			Replacement: `unit: "A"`,
			Desc:        "Normalize unit 'Amperes' to canonical standard 'A'",
		},
		{
			Pattern:     regexp.MustCompile(`(?i)unit:\s*["']?Watts["']?`),
			Replacement: `unit: "W"`,
			Desc:        "Normalize unit 'Watts' to canonical standard 'W'",
		},
	}
)

func RunMigration(opts MigrateOptions) (*MigrateResult, error) {
	result := &MigrateResult{}

	targetDir := opts.TargetDir
	if targetDir == "" {
		targetDir = "."
	}

	err := filepath.WalkDir(targetDir, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}

		// Skip hidden dirs, build dirs, and version control
		if d.IsDir() {
			name := d.Name()
			if strings.HasPrefix(name, ".") || name == "build" || name == "node_modules" || name == "third-party" {
				return filepath.SkipDir
			}
			return nil
		}

		ext := strings.ToLower(filepath.Ext(path))
		if ext == ".c" || ext == ".h" || ext == ".yaml" || ext == ".yml" {
			result.ScannedFiles++
			fileModified, fileIssues, err := processFile(path, ext, opts.Fix)
			if err != nil {
				return err
			}
			if len(fileIssues) > 0 {
				result.IssuesFound += len(fileIssues)
				result.Issues = append(result.Issues, fileIssues...)
				if fileModified {
					result.IssuesFixed += len(fileIssues)
					result.ModifiedFiles = append(result.ModifiedFiles, path)
				}
			}
		}
		return nil
	})

	if err != nil {
		return nil, err
	}

	return result, nil
}

func processFile(path string, ext string, fix bool) (bool, []MigrationIssue, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return false, nil, err
	}

	content := string(data)
	newContent := content
	var issues []MigrationIssue

	if ext == ".c" || ext == ".h" {
		for _, rule := range cHeaderRules {
			if rule.Pattern.MatchString(newContent) {
				issues = append(issues, MigrationIssue{
					File:        path,
					Description: rule.Desc,
				})
				newContent = rule.Pattern.ReplaceAllString(newContent, rule.Replacement)
			}
		}
	} else if ext == ".yaml" || ext == ".yml" {
		for _, rule := range yamlRules {
			if rule.Pattern.MatchString(newContent) {
				issues = append(issues, MigrationIssue{
					File:        path,
					Description: rule.Desc,
				})
				newContent = rule.Pattern.ReplaceAllString(newContent, rule.Replacement)
			}
		}
	}

	if newContent != content {
		if fix {
			if err := os.WriteFile(path, []byte(newContent), 0644); err != nil {
				return false, issues, err
			}
			return true, issues, nil
		}
		return false, issues, nil
	}

	return false, issues, nil
}

func PrintReport(result *MigrateResult, fix bool) {
	fmt.Printf("🔍 EdgeOS Migration Scanner Report\n")
	fmt.Printf("   • Scanned files: %d\n", result.ScannedFiles)
	fmt.Printf("   • Issues found : %d\n", result.IssuesFound)

	if len(result.Issues) == 0 {
		fmt.Printf("   ✅ Clean! No deprecated SDK headers or legacy contract schemas detected.\n")
		return
	}

	for idx, issue := range result.Issues {
		status := "⚠️ [DETECTED]"
		if fix {
			status = "✅ [MIGRATED]"
		}
		fmt.Printf("   %s Issue #%d (%s): %s\n", status, idx+1, issue.File, issue.Description)
	}

	if fix {
		fmt.Printf("\n✨ Successfully migrated %d files (%d issues resolved).\n", len(result.ModifiedFiles), result.IssuesFixed)
	} else {
		fmt.Printf("\n💡 Run 'edge-cli migrate --fix' to automatically apply these migration fixes.\n")
	}
}
