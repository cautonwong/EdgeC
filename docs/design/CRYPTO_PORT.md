# 密码学端口与 OpenSSL 依赖

状态：设计记录（2026-11-15）。只定端口和迁移顺序，不改代码。

## 1. 现状

仓库已有 `foundation/include/hal_crypto.h` 的 `crypto_ops_t` 端口，包含摘要、HMAC、AES-GCM、ECDSA、ECDH、证书和 TRNG 方法。`hal_crypto_register()` 只保存一个后端指针，`hal_crypto_get_ops()` 返回它。

但目前没有生产后端注册：

- `hal_crypto_register` 的调用者只有测试里的 mock。
- `modules/edge_stream/src/stage_vpp_logger.c` 取不到后端时把签名写成 `UNSIGNED`。
- `foundation/src/protocol/dlms_security.c` 的 `dlms_security_apply` 在没有后端时直接返回 0，要求加密的路径被静默跳过；其余函数返回 -1。

同时有五个文件直接包含 OpenSSL：

| 文件 | 用途 | 是否走端口 |
|---|---|---|
| `modules/edge_mgmt/src/edge_eapp.c` | SHA-256 key id、Ed25519 签名/验签、公钥派生 | 否 |
| `adapters/sqlite_adapters/src/edge_backup.c` | SHA-256 流式摘要 | 否 |
| `adapters/sqlite_adapters/src/edge_digest.c` | SHA-256 文件摘要 | 否 |
| `modules/edge_stream/src/wasm_runtime/wasm_mgr.c` | RSA PKCS1v15-SHA256 验签 | 否 |
| `modules/edge_stream/src/stage_vpp_logger.c` | HMAC | 是，但后端未注册 |

CMake 侧，`CMakeLists.txt:37` 顶层 `find_package(OpenSSL REQUIRED)`，`edge_stream`、`edge_mgmt`、`edge_sqlite_adapters` 分别链接 `OpenSSL::Crypto`。因此没有 OpenSSL 的平台无法裁剪，即使产品不需要签名和加密。

## 2. 决定

保留 `hal_crypto.h` 作为唯一密码学端口，不新增第二个抽象。补齐端口缺少的方法，写一个 OpenSSL 后端适配器，由组合根注册。模块不再直接包含 `<openssl/*>`。

端口新增三类方法：

```c
int (*ed25519_verify)(const uint8_t pub[32], const uint8_t *msg, size_t len,
                      const uint8_t sig[64]);
int (*ed25519_sign)(const uint8_t priv[32], const uint8_t *msg, size_t len,
                    uint8_t sig[64]);
int (*ed25519_public_from_private)(const uint8_t priv[32], uint8_t pub[32]);
int (*rsa_pkcs1v15_sha256_verify)(const char *public_key_pem, const uint8_t *msg,
                                  size_t len, const uint8_t *sig, size_t sig_len);
```

现有 `hash_init/hash_update/hash_finish` 覆盖流式摘要，`hmac_calculate` 覆盖 VPP 审计。不需要为这两个用途加新方法。

## 3. 迁移顺序

1. **端口后端**：新建 `adapters/crypto_openssl`，实现 `crypto_ops_t` 的 OpenSSL 版本；组合根在启动时 `hal_crypto_register()`。先覆盖 hash/HMAC/Ed25519/RSA 验签，AES-GCM/ECDSA/ECDH 保持未实现并在 `get_capabilities` 中如实报告。
2. **摘要用户**：`edge_digest.c`、`edge_backup.c` 改用 `hash_init/update/finish`，删除 `<openssl/evp.h>`。
3. **应用包**：`edge_eapp.c` 改用 `ed25519_*` 方法；SHA-256 key id 走 `hash_calculate`。
4. **wasm 签名**：`wasm_mgr.c` 改用 `rsa_pkcs1v15_sha256_verify`；PEM 解析留在适配器里。
5. **构建**：模块 CMake 去掉 `OpenSSL::Crypto`；只有 `crypto_openssl` 链接 OpenSSL。顶层 `find_package(OpenSSL REQUIRED)` 移到该适配器，按产品需要启用。

每一步都保留现有测试，另加端口层单元测试。

## 4. 边界

- 模块只依赖 `hal_crypto.h`，不依赖 OpenSSL、mbedTLS 或具体 key 结构。
- 适配器负责 PEM/DER 解析、错误码映射和密钥长度校验。
- 端口不暴露 `EVP_PKEY`、`mbedtls_pk_context` 等后端类型。
- 没有后端时，需要签名的路径必须明确失败，不能输出 `UNSIGNED` 当作成功。

## 5. 验收

- 五个直接 OpenSSL 用户全部改走端口，`modules/` 和 `foundation/protocol` 不再出现 `<openssl/`。
- `crypto_openssl` 的 hash/HMAC/Ed25519/RSA 测试通过，包含错误签名、错误长度和空消息。
- 没有注册后端时，`stage_vpp_logger` 不再写 `UNSIGNED`，`dlms_security_apply` 不再静默跳过加密，两者都必须明确失败并计数。
- 一个不链接 OpenSSL 的构建配置仍能构建不依赖密码学的产品子集。
- 全量测试和 ASAN 通过。

## 6. 依赖审计记录

| 位置 | 现状 | 结论 |
|---|---|---|
| `CMakeLists.txt:37` | 顶层 `find_package(OpenSSL REQUIRED)` | 所有配置都被强制依赖 |
| `modules/edge_stream/CMakeLists.txt` | 链接 `OpenSSL::Crypto` | 因为 `wasm_mgr.c` 和 `stage_vpp_logger.c` |
| `modules/edge_mgmt/CMakeLists.txt` | 链接 `OpenSSL::Crypto` | 因为 `edge_eapp.c` |
| `adapters/sqlite_adapters/CMakeLists.txt` | 链接 `OpenSSL::Crypto` | 因为 `edge_backup.c`、`edge_digest.c` |
| `cmd/edgeos-launcher/CMakeLists.txt` | 只链接 `edge_foundation`、`edge_common_interface` | 不直接依赖 OpenSSL |
| `cmd/app-host/CMakeLists.txt` | 只链接 `iwasm::vmlib` | 不直接依赖 OpenSSL |

只把顶层 `find_package` 改成可选不会去掉任何链接，组件仍会拉 OpenSSL。真正的条件化必须等 `crypto_openssl` 适配器落地、模块改走 `hal_crypto.h` 之后再做。当前没有低风险的一行式改动。

## 7. 未决

- 硬件密码后端（TPM、SE）的能力位图和密钥句柄还没有设计。
- FIPS 或国密要求是否需要独立后端，取决于目标市场。
- `crypto_ops_t` 的 `ecdsa_*` 方法参数不含 hash 算法和签名长度，迁移到真实后端前需要重新评审签名。
