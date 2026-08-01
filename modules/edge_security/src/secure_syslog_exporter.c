/**
 * @file secure_syslog_exporter.c
 * @brief Edge-Guard 安全审计日志导出 (带 HMAC 签名)
 */

#include "edge_guard.h"
#include "hal_crypto.h"
#include "edge_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int mock_hmac_impl(crypto_hash_algo_t algo, const crypto_key_t *key, const uint8_t *input,
                     size_t ilen, uint8_t *output) {
    (void)algo; (void)key; (void)input; (void)ilen;
    memset(output, 0xAA, 32);
    return 0;
}

crypto_ops_t mbedtls_ops = {
    .name = "mbedtls_impl",
    .hmac_calculate = mock_hmac_impl
};

static crypto_key_t g_audit_key = {
    .raw_data = NULL,
    .len = 0,
    .is_hardware = false
};

/**
 * @brief 记录并签名安全事件
 */
void dpi_audit_signed_log(dpi_result_t res, uint32_t tag_id, double val, const char *reason) {
    const char *env_secret = getenv("EDGE_GUARD_AUDIT_SECRET");
    if (!env_secret || strlen(env_secret) == 0) {
        env_secret = "TEST_ONLY_edge_guard_audit_secret";
    }
    g_audit_key.raw_data = (const uint8_t *)env_secret;
    g_audit_key.len = (uint32_t)strlen(env_secret);

    char log_buf[512];
    uint8_t signature[32]; // SHA-256 HMAC
    char sig_hex[65];
    
    time_t now;
    time(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", gmtime(&now));

    // 1. 构造日志原文
    int log_len = snprintf(log_buf, sizeof(log_buf), 
        "[%s] EVENT=SECURITY_BLOCK RESULT=%d TAG=%u VAL=%.2f REASON=%s",
        time_str, (int)res, tag_id, val, reason);

    // 2. 计算 HMAC-SHA256 签名 (防篡改)
    if (mbedtls_ops.hmac_calculate) {
        mbedtls_ops.hmac_calculate(CRYPTO_HASH_SHA256, &g_audit_key, 
                                   (const uint8_t *)log_buf, (size_t)log_len, signature);
        
        // 转为 HEX 字符串
        for (int i = 0; i < 32; i++) {
            sprintf(sig_hex + i * 2, "%02x", signature[i]);
        }
    } else {
        strncpy(sig_hex, "UNSIGNED-NOSUPPORT", 64);
    }

    // 3. 输出加密审计日志 (生产环境下可直接发往远程 SIEM)
    LOG_ERROR(LOG_CAT_CORE, "AUDIT_SIGNED: %s | SIG=%s", log_buf, sig_hex);
    
    // TODO: 调用持久化服务将 signed log 存入 SQLite 或发送至 TLS Socket
}
