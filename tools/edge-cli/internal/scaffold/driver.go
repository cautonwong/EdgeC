package scaffold

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

type DriverOptions struct {
	Name        string
	Type        string // serial, tcp, can, custom
	Protocol    string // modbus, dlt645, iec104, custom
	OutputDir   string
	BaseDir     string
}

func CreateDriver(opts DriverOptions) error {
	drvName := strings.ToLower(strings.ReplaceAll(opts.Name, "-", "_"))
	if drvName == "" {
		return fmt.Errorf("driver name cannot be empty")
	}

	drvType := strings.ToLower(opts.Type)
	if drvType == "" {
		drvType = "serial"
	}

	protocol := strings.ToLower(opts.Protocol)
	if protocol == "" {
		protocol = "custom"
	}

	baseDir := opts.BaseDir
	if baseDir == "" {
		baseDir = "."
	}

	includeDir := filepath.Join(baseDir, "modules", "edge_io", "include", "drivers")
	srcDir := filepath.Join(baseDir, "modules", "edge_io", "src", "drivers")
	bindingDir := filepath.Join(baseDir, "contracts", "bindings", protocol)

	if err := os.MkdirAll(includeDir, 0755); err != nil {
		return fmt.Errorf("failed to create directory %s: %w", includeDir, err)
	}
	if err := os.MkdirAll(srcDir, 0755); err != nil {
		return fmt.Errorf("failed to create directory %s: %w", srcDir, err)
	}
	if err := os.MkdirAll(bindingDir, 0755); err != nil {
		return fmt.Errorf("failed to create directory %s: %w", bindingDir, err)
	}

	upperName := strings.ToUpper(drvName)
	upperType := strings.ToUpper(drvType)

	// 1. Header file
	headerTmpl := `/**
 * @file {{DRV_NAME}}_driver.h
 * @brief {{UPPER_NAME}} Edge Driver Interface
 */

#ifndef EDGE_{{UPPER_NAME}}_DRIVER_H
#define EDGE_{{UPPER_NAME}}_DRIVER_H

#include "edge_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief {{UPPER_NAME}} driver descriptor
 */
extern const edge_driver_t {{DRV_NAME}}_driver;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_{{UPPER_NAME}}_DRIVER_H */
`
	headerContent := strings.ReplaceAll(headerTmpl, "{{DRV_NAME}}", drvName)
	headerContent = strings.ReplaceAll(headerContent, "{{UPPER_NAME}}", upperName)

	headerPath := filepath.Join(includeDir, drvName+"_driver.h")
	if err := os.WriteFile(headerPath, []byte(headerContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", headerPath, err)
	}

	// 2. Source file
	srcTmpl := `/**
 * @file {{DRV_NAME}}_driver.c
 * @brief {{UPPER_NAME}} Edge Driver Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drivers/{{DRV_NAME}}_driver.h"
#include "edge_log.h"

typedef struct {
    int fd;
    char port[64];
    uint32_t baudrate;
} {{DRV_NAME}}_driver_ctx_t;

static int {{DRV_NAME}}_drv_init(void) {
    LOG_INFO(LOG_CAT_IO, "Initializing {{DRV_NAME}} driver");
    return 0;
}

static int {{DRV_NAME}}_drv_open(edge_device_t* dev, const char* uri) {
    if (!dev || !uri) return -1;
    {{DRV_NAME}}_driver_ctx_t* ctx = calloc(1, sizeof({{DRV_NAME}}_driver_ctx_t));
    if (!ctx) return -1;
    strncpy(ctx->port, uri, sizeof(ctx->port) - 1);
    ctx->fd = 1; // Simulated valid descriptor
    dev->driver_data = ctx;
    LOG_INFO(LOG_CAT_IO, "Opened {{DRV_NAME}} device on %s", uri);
    return 0;
}

static int {{DRV_NAME}}_drv_close(edge_device_t* dev) {
    if (!dev || !dev->driver_data) return -1;
    free(dev->driver_data);
    dev->driver_data = NULL;
    LOG_INFO(LOG_CAT_IO, "Closed {{DRV_NAME}} device");
    return 0;
}

static int {{DRV_NAME}}_drv_read(edge_device_t* dev, void* buf, size_t len) {
    if (!dev || !buf) return -1;
    (void)len;
    return 0;
}

static int {{DRV_NAME}}_drv_write(edge_device_t* dev, const void* buf, size_t len) {
    if (!dev || !buf) return -1;
    (void)len;
    return 0;
}

static int {{DRV_NAME}}_drv_ioctl(edge_device_t* dev, uint32_t cmd, void* arg) {
    (void)dev;
    (void)cmd;
    (void)arg;
    return 0;
}

static const edge_driver_ops_t {{DRV_NAME}}_ops = {
    .init = {{DRV_NAME}}_drv_init,
    .open = {{DRV_NAME}}_drv_open,
    .close = {{DRV_NAME}}_drv_close,
    .read = {{DRV_NAME}}_drv_read,
    .write = {{DRV_NAME}}_drv_write,
    .ioctl = {{DRV_NAME}}_drv_ioctl,
};

const edge_driver_t {{DRV_NAME}}_driver = {
    .name = "{{DRV_NAME}}",
    .version = "1.0.0",
    .type = EDGE_DRIVER_TYPE_{{UPPER_TYPE}},
    .ops = &{{DRV_NAME}}_ops,
};
`
	srcContent := strings.ReplaceAll(srcTmpl, "{{DRV_NAME}}", drvName)
	srcContent = strings.ReplaceAll(srcContent, "{{UPPER_NAME}}", upperName)
	srcContent = strings.ReplaceAll(srcContent, "{{UPPER_TYPE}}", upperType)

	srcPath := filepath.Join(srcDir, drvName+"_driver.c")
	if err := os.WriteFile(srcPath, []byte(srcContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", srcPath, err)
	}

	// 3. Binding YAML
	bindingTmpl := `# [EdgeOS Driver Binding] {{UPPER_NAME}}
binding: "{{UPPER_NAME}}"
driver: "{{DRV_NAME}}"
protocol: "{{PROTOCOL}}"

points:
  - id: 1001
    name: "VOLTAGE_PHASE_A"
    address: "0x0001"
    register_type: "holding"
    factor: 0.1
`
	bindingContent := strings.ReplaceAll(bindingTmpl, "{{DRV_NAME}}", drvName)
	bindingContent = strings.ReplaceAll(bindingContent, "{{UPPER_NAME}}", upperName)
	bindingContent = strings.ReplaceAll(bindingContent, "{{PROTOCOL}}", protocol)

	bindingPath := filepath.Join(bindingDir, drvName+".yaml")
	if err := os.WriteFile(bindingPath, []byte(bindingContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", bindingPath, err)
	}

	fmt.Printf("✅ Successfully created driver template for '%s':\n", drvName)
	fmt.Printf("  • Header file  : %s\n", headerPath)
	fmt.Printf("  • Source file  : %s\n", srcPath)
	fmt.Printf("  • Binding YAML : %s\n", bindingPath)
	return nil
}
