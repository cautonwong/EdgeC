#ifndef EDGEOS_CONSOLE_RBAC_H
#define EDGEOS_CONSOLE_RBAC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Industrial RBAC Roles aligned with IEC 62443 & Power Grid Secondary Security standards
 */
typedef enum {
    CONSOLE_ROLE_ANONYMOUS = 0,
    CONSOLE_ROLE_OBSERVER  = 1,   /* 监视员: 仅限查看业务看板，禁止打开 Web 控制台 */
    CONSOLE_ROLE_OPERATOR  = 2,   /* 运行员: 仅限执行只读/诊断命令，写/配置/控制指令即时阻断 */
    CONSOLE_ROLE_ENGINEER  = 3,   /* 工程师: 允许全量调试命令执行，高危指令需双人复核 */
    CONSOLE_ROLE_ADMIN     = 4    /* 管理员: 允许全量调试与系统管理，高危指令需双人复核 */
} console_role_t;

/**
 * Command Risk Evaluation Level
 */
typedef enum {
    CONSOLE_RISK_LOW      = 0,   /* 只读/诊断/状态查询 (read, stats, peers, list, top, help) */
    CONSOLE_RISK_MEDIUM   = 1,   /* 常规参数写入与非关键通道开关 */
    CONSOLE_RISK_HIGH     = 2,   /* 遥控跳闸、合闸、点位强制写入、DAG 流计算启停 */
    CONSOLE_RISK_CRITICAL = 3    /* 系统重启、关机、存储格式化、主干参数重置 */
} console_risk_level_t;

/**
 * Initialize the Console RBAC engine
 */
int console_rbac_init(void);

/**
 * Parse role string to enum
 */
console_role_t console_rbac_parse_role(const char *role_str);

/**
 * Convert role enum to human-readable string
 */
const char* console_rbac_role_to_str(console_role_t role);

/**
 * Check whether Web Console access is permitted for this role
 * (Observer is completely blocked)
 */
bool console_rbac_is_console_allowed(console_role_t role);

/**
 * Evaluate if a specific command line is allowed for the given role
 *
 * @param role The caller's role
 * @param cmd The command line string
 * @param err_msg Output buffer for error explanation if denied
 * @param max_len Size of err_msg buffer
 * @return true if permitted, false if denied (e.g. Operator attempting write)
 */
bool console_rbac_is_command_allowed(console_role_t role, const char *cmd, char *err_msg, size_t max_len);

/**
 * Evaluate command risk level and return risk explanation
 */
console_risk_level_t console_rbac_evaluate_risk(const char *cmd, char *risk_reason, size_t max_len);

/**
 * Convenience check for high-risk / critical commands requiring Two-Man Rule
 */
bool console_rbac_is_high_risk(const char *cmd, char *risk_reason, size_t max_len);

/**
 * Verify Two-Man Rule authorization credentials
 *
 * @param operator_user Name of operator initiating command
 * @param operator_role Role of operator
 * @param supervisor_user Name of second operator / supervisor
 * @param supervisor_pwd_or_token Supervisor password or 6-digit TOTP / authorization token
 * @param action_cmd The command to be executed
 * @param out_err Error message buffer if verification fails
 * @param max_len Buffer length
 * @return true if verified, false if invalid
 */
bool console_rbac_verify_two_man_rule(const char *operator_user, console_role_t operator_role,
                                      const char *supervisor_user, const char *supervisor_pwd_or_token,
                                      const char *action_cmd, char *out_err, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_CONSOLE_RBAC_H */
