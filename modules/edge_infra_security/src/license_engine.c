/**
 * @file license_engine.c
 * @brief SDK Tiering and License Verification Implementation
 */

#include "edge_license.h"
#include <stdio.h>
#include <string.h>

#define SEC_LOG_INFO(fmt, ...)  fprintf(stderr, "[SEC][INFO] " fmt "\n", ##__VA_ARGS__)
#define SEC_LOG_WARN(fmt, ...)  fprintf(stderr, "[SEC][WARN] " fmt "\n", ##__VA_ARGS__)
#define SEC_LOG_ERROR(fmt, ...) fprintf(stderr, "[SEC][ERROR] " fmt "\n", ##__VA_ARGS__)

static bool g_license_active = false;
static edgec_tier_t g_runtime_tier = TIER_BASIC;

int license_engine_init(const char *license_key) {
    // This is a placeholder for actual cryptographic verification.
    // In production, this would use mbedtls_ops to verify a signed JWT or hardware token.
    
    SEC_LOG_INFO("Licensing Engine: Booting...");

    if (!license_key || strlen(license_key) == 0) {
        SEC_LOG_WARN("Licensing Engine: No key provided. Reverting to BASIC tier.");
        g_runtime_tier = TIER_BASIC;
        g_license_active = true;
        return 0;
    }

    // Mock validation logic
    if (strstr(license_key, "ENT-")) {
        g_runtime_tier = TIER_ENTERPRISE;
    } else if (strstr(license_key, "PRO-")) {
        g_runtime_tier = TIER_PROFESSIONAL;
    } else {
        g_runtime_tier = TIER_BASIC;
    }

    // Cross-check runtime license vs compiled tier
    if (g_runtime_tier < CURRENT_SDK_TIER) {
        SEC_LOG_ERROR("Licensing Engine: License tier (%d) does not permit running this binary compiled for tier (%d)", 
                 (int)g_runtime_tier, (int)CURRENT_SDK_TIER);
        return -1; // Abort boot
    }

    g_license_active = true;
    SEC_LOG_INFO("Licensing Engine: Validated successfully for Tier %d. Compiled Tier: %d", 
             (int)g_runtime_tier, (int)CURRENT_SDK_TIER);

    return 0;
}

bool license_engine_check_feature(const char *feature_name) {
    if (!g_license_active) return false;

    if (strcmp(feature_name, "WASM") == 0) {
        return license_supports_wasm() && (g_runtime_tier >= TIER_ENTERPRISE);
    }
    
    if (strcmp(feature_name, "DSP") == 0) {
        return license_supports_dsp() && (g_runtime_tier >= TIER_PROFESSIONAL);
    }

    // Default to true for base features
    return true;
}
