package migrate

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestMigrateScanAndFix(t *testing.T) {
	tmpDir, err := os.MkdirTemp("", "edge_migrate_test_*")
	if err != nil {
		t.Fatalf("Failed to create temp dir: %v", err)
	}
	defer os.RemoveAll(tmpDir)

	// Create legacy C file
	cFile := filepath.Join(tmpDir, "legacy_driver.c")
	legacyCContent := `#include "edgeos.h"
#include "datahub.h"
#include "edge_driver.h"

int main() { return 0; }
`
	if err := os.WriteFile(cFile, []byte(legacyCContent), 0644); err != nil {
		t.Fatalf("Failed to write legacy C file: %v", err)
	}

	// Create legacy YAML file
	yamlFile := filepath.Join(tmpDir, "legacy_contract.yaml")
	legacyYamlContent := `product: "legacy_bess"
include: "contracts/domains/power.yaml"
tags:
  - id: 1001
    name: "VOLTAGE"
    type: "float_32"
    unit: "Volts"
`
	if err := os.WriteFile(yamlFile, []byte(legacyYamlContent), 0644); err != nil {
		t.Fatalf("Failed to write legacy YAML file: %v", err)
	}

	// Test 1: Dry-Run Scan
	resDryRun, err := RunMigration(MigrateOptions{
		TargetDir: tmpDir,
		Fix:       false,
	})
	if err != nil {
		t.Fatalf("RunMigration dry-run failed: %v", err)
	}

	if resDryRun.IssuesFound < 4 {
		t.Errorf("Expected at least 4 issues found in dry-run, got %d", resDryRun.IssuesFound)
	}
	if resDryRun.IssuesFixed != 0 {
		t.Errorf("Expected 0 issues fixed in dry-run, got %d", resDryRun.IssuesFixed)
	}

	// Verify files not modified yet
	dryCData, _ := os.ReadFile(cFile)
	if !strings.Contains(string(dryCData), `#include "edgeos.h"`) {
		t.Errorf("File should not be modified in dry-run")
	}

	// Test 2: Auto-Fix Migration
	resFix, err := RunMigration(MigrateOptions{
		TargetDir: tmpDir,
		Fix:       true,
	})
	if err != nil {
		t.Fatalf("RunMigration fix failed: %v", err)
	}

	if resFix.IssuesFixed == 0 {
		t.Errorf("Expected issues to be fixed, got 0")
	}

	// Verify migrated C file
	fixedCData, _ := os.ReadFile(cFile)
	fixedCStr := string(fixedCData)
	if !strings.Contains(fixedCStr, `#include <edgeos/edgeos.h>`) {
		t.Errorf("Expected #include <edgeos/edgeos.h>, got: %s", fixedCStr)
	}
	if !strings.Contains(fixedCStr, `#include <edgeos/datahub.h>`) {
		t.Errorf("Expected #include <edgeos/datahub.h>, got: %s", fixedCStr)
	}
	if !strings.Contains(fixedCStr, `#include <edgeos/driver.h>`) {
		t.Errorf("Expected #include <edgeos/driver.h>, got: %s", fixedCStr)
	}

	// Verify migrated YAML file
	fixedYamlData, _ := os.ReadFile(yamlFile)
	fixedYamlStr := string(fixedYamlData)
	if !strings.Contains(fixedYamlStr, `includes:`) {
		t.Errorf("Expected includes:, got: %s", fixedYamlStr)
	}
	if !strings.Contains(fixedYamlStr, `type: "float32"`) {
		t.Errorf("Expected type: \"float32\", got: %s", fixedYamlStr)
	}
	if !strings.Contains(fixedYamlStr, `unit: "V"`) {
		t.Errorf("Expected unit: \"V\", got: %s", fixedYamlStr)
	}

	// Test 3: Idempotent re-run on clean directory
	resClean, err := RunMigration(MigrateOptions{
		TargetDir: tmpDir,
		Fix:       false,
	})
	if err != nil {
		t.Fatalf("RunMigration on clean dir failed: %v", err)
	}
	if resClean.IssuesFound != 0 {
		t.Errorf("Expected 0 issues on clean directory, got %d", resClean.IssuesFound)
	}
}
