package scaffold

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

type ProductOptions struct {
	Name        string
	OutputDir   string
	BaseDir     string
	Description string
}

func CreateProduct(opts ProductOptions) error {
	prodName := strings.ToLower(strings.ReplaceAll(opts.Name, "-", "_"))
	if prodName == "" {
		return fmt.Errorf("product name cannot be empty")
	}

	baseDir := opts.BaseDir
	if baseDir == "" {
		baseDir = "."
	}

	cmdDir := filepath.Join(baseDir, "cmd", prodName)
	contractsDir := filepath.Join(baseDir, "contracts", "products")

	if err := os.MkdirAll(cmdDir, 0755); err != nil {
		return fmt.Errorf("failed to create directory %s: %w", cmdDir, err)
	}
	if err := os.MkdirAll(contractsDir, 0755); err != nil {
		return fmt.Errorf("failed to create directory %s: %w", contractsDir, err)
	}

	// 1. contracts/products/<name>.yaml
	prodYamlPath := filepath.Join(contractsDir, prodName+".yaml")
	prodYamlContent := fmt.Sprintf(`# [EdgeOS Product Definition] %s
product: "%s"

# 1. 语义层继承 (Semantic Domains)
include:
  - "contracts/schema/domains/power.yaml"
  - "contracts/schema/domains/energy.yaml"
  - "contracts/schema/domains/system.yaml"
  - "contracts/bindings/modbus/acrel_p80.yaml"

# 2. 默认行为注入 (Default Behaviors)
defaults:
  - match: "traits:metrology"
    attributes:
      poll_interval_ms: 1000
      ui: { group: "Grid_Params" }

# 3. 行为定制 (Overrides)
tags:
  - id: 1001 # VOLTAGE_PHASE_A
    attributes:
      ui: { group: "Grid_Params", min: 0, max: 500, precision: 1 }
`, strings.ToUpper(prodName), prodName)

	if err := os.WriteFile(prodYamlPath, []byte(prodYamlContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", prodYamlPath, err)
	}

	// 2. cmd/<name>/manifest.yaml
	manifestPath := filepath.Join(cmdDir, "manifest.yaml")
	manifestContent := fmt.Sprintf(`# Local feature manifest for %s
features:
  telemetry: true
  storage_wal: true
`, prodName)
	if err := os.WriteFile(manifestPath, []byte(manifestContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", manifestPath, err)
	}

	// 3. cmd/<name>/CMakeLists.txt
	cmakePath := filepath.Join(cmdDir, "CMakeLists.txt")
	cmakeContent := fmt.Sprintf(`# EdgeOS Product CMake Configuration: %s
set(PROD_NAME %s)

# 1. 契约生成与查表库 (自动生成 edge_driver_registry.h/c 等)
edge_add_contract(
    contract_${PROD_NAME}
    "${CMAKE_SOURCE_DIR}/contracts/products/${PROD_NAME}.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.yaml"
)

# 2. 声明主产品可执行文件
add_executable(${PROD_NAME} main.c)
set_target_properties(${PROD_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
)

# 3. 链接 SDK 模块与生成式契约库
target_link_libraries(${PROD_NAME} PRIVATE
    -Wl,--start-group
    edge_infra
    edge_io
    edge_stream
    edge_algo
    edge_security
    edge_core
    edge_protocol
    contract_${PROD_NAME}
    -Wl,--end-group
    edge_common_interface
    pthread dl rt m
)
`, prodName, prodName)
	if err := os.WriteFile(cmakePath, []byte(cmakeContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", cmakePath, err)
	}

	// 4. cmd/<name>/main.c (Clean Architecture with Contract-Generated Driver Registry)
	mainPath := filepath.Join(cmdDir, "main.c")
	mainContent := fmt.Sprintf(`/**
 * @file main.c
 * @brief %s Product Main Entry (Clean Architecture / Contract-Driven)
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "edge_core.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver_registry.h"

static volatile sig_atomic_t g_running = 1;

static void sig_handler(int sig) {
    (void)sig;
    g_running = 0;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("=========================================================\\n");
    printf("🚀 Starting EdgeOS Product: %s\\n", "%s");
    printf("=========================================================\\n");

    // 1. 初始化 DataHub
    datahub_config_t dh_cfg = {
        .backend = DATAHUB_BACKEND_SHM,
        .max_tags = 2048,
    };
    datahub_t* dh = datahub_create(&dh_cfg);
    if (!dh) {
        fprintf(stderr, "[-] Failed to initialize DataHub\\n");
        return -1;
    }

    // 2. 初始化驱动管理器并注入生成式驱动注册表 (零手工 extern)
    driver_manager_t* drv_mgr = driver_manager_create();
    if (!drv_mgr) {
        fprintf(stderr, "[-] Failed to create driver manager\\n");
        datahub_destroy(dh);
        return -1;
    }

    int reg_rc = edge_driver_registry_init(drv_mgr);
    printf("[+] Generated driver registry initialized (result: %%d, drivers: %%zu)\\n",
           reg_rc, g_edge_driver_registry_count);

    printf("[+] Product %s is ready and operational.\\n", "%s");

    // 运行主循环 (若单次运行则退出)
    if (getenv("EDGEOS_RUN_ONCE")) {
        printf("[+] EDGEOS_RUN_ONCE mode: exiting cleanly.\\n");
    } else {
        while (g_running) {
            usleep(100000); // 100ms
        }
    }

    // 3. 优雅清理与退出
    driver_manager_destroy(drv_mgr);
    datahub_destroy(dh);
    printf("[-] Product %s stopped cleanly.\\n", "%s");
    return 0;
}
`, strings.ToUpper(prodName), prodName, prodName, prodName, prodName)
	if err := os.WriteFile(mainPath, []byte(mainContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", mainPath, err)
	}

	// 5. cmd/<name>/README.md
	readmePath := filepath.Join(cmdDir, "README.md")
	readmeContent := fmt.Sprintf(`# EdgeOS Product: %s

## Overview
Auto-generated product scaffold created by `+"`edge-cli create-product`"+`.

## Build
`+"```bash"+`
cmake -B build -GNinja
ninja -C build %s
./build/products/%s/bin/%s
`+"```"+`
`, prodName, prodName, prodName, prodName)
	if err := os.WriteFile(readmePath, []byte(readmeContent), 0644); err != nil {
		return fmt.Errorf("failed to write %s: %w", readmePath, err)
	}

	fmt.Printf("✅ Successfully created product scaffold for '%s':\n", prodName)
	fmt.Printf("  • Contract YAML : %s\n", prodYamlPath)
	fmt.Printf("  • Manifest      : %s\n", manifestPath)
	fmt.Printf("  • CMakeLists.txt: %s\n", cmakePath)
	fmt.Printf("  • Entry main.c  : %s\n", mainPath)
	fmt.Printf("  • README.md     : %s\n", readmePath)
	return nil
}
