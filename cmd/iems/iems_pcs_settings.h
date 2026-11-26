/**
 * @file iems_pcs_settings.h
 * @brief P2-02a: typed PCS/EMS settings loaded from converted ems/lc configs.
 *
 * Data shape: every non-deprecated key of config/schema/ems.schema.json and
 * the object branch of lc.schema.json gets a named field; there is no dynamic
 * map. Defaults are zero values: the schemas define no defaults, and v3's
 * runtime static defaults (e.g. SOCmax=100, SOCmaxErr=5) are NOT applied here.
 * The golden files' values are test placeholders. Fields are named exactly
 * like the JSON keys so the mapping stays reviewable.
 *
 * Trust boundary: only P1-03b converted artifacts (schema_version "1.0.0")
 * are accepted. The loader revalidates keys and representation itself
 * (integers must fit int32, doubles must be finite), rejects deprecated keys
 * (the converter drops them), and never resolves secrets. The schemas carry no
 * min/max/enum: semantic bounds (SOC 0..100 etc.) are deliberately not
 * enforced here - that belongs to P2-03/P5-03 or a schema update.
 */

#ifndef IEMS_PCS_SETTINGS_H
#define IEMS_PCS_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "iems_config_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ems.schema.json: 18 non-deprecated scalar keys + tou (maxItems 6). */
#define IEMS_EMS_SETTINGS_SCALAR_KEYS 18u
#define IEMS_EMS_TOU_MAX 6u
#define IEMS_EMS_TOU_KEYS 8u

/* lc.schema.json object branch: 24 keys. */
#define IEMS_LC_SETTINGS_KEYS 24u

typedef struct {
    int32_t StHour;
    int32_t StMin;
    int32_t EnHour;
    int32_t EnMin;
    int32_t Power;
    int32_t RePower;
    int32_t SOCUpperLimit;
    int32_t SOCLowerLimit;
} iems_ems_tou_t;

typedef struct {
    /* 13 个保留旧键（golden ems/ems_cfg.json 的形态）。 */
    int32_t Pmlmax;
    int32_t K1;
    int32_t K2;
    int32_t EnProtectDOD;
    int32_t EnBmsAlarm;
    int32_t SOCmax;
    int32_t SOCmin;
    int32_t Pset;
    int32_t Adjpreriod;
    int32_t SleepMode;
    double SocKeepActiveSOC;
    double SocKeepMargin;
    double SocKeepChargePower;
    /* schema 的当前读取键：转换源若已用新名，原样保留。 */
    int32_t PvMode;
    int32_t CabPower;
    int32_t ENRPComp;
    double SetPowerFactor;
    int32_t SocKeepModeEnable;
    /* tou：仅保留已出现的元素，tou_count <= 6。 */
    iems_ems_tou_t tou[IEMS_EMS_TOU_MAX];
    uint32_t tou_count;
} iems_ems_settings_t;

typedef struct {
    int32_t EnBmsAlarm;
    int32_t EnProtectDOD;
    int32_t BmsPowerLimit;
    int32_t PcsAutoTurn;
    int32_t PcsAutoTurnOffTime;
    int32_t EnIoProtect;
    int32_t SOCmax;
    int32_t SOCmin;
    int32_t PowerLimit;
    int32_t TMSCtrlTempCold;
    int32_t TMSCtrlTempHot;
    int32_t TMSCtrlState;
    int32_t AirCondStartStopCtrl;
    double singleMaxVoltage;
    double singleMinVoltage;
    int32_t enSingleVoltage;
    double singleMaxVoltageErr;
    double singleMinVoltageErr;
    int32_t SOCminErr;
    int32_t SOCmaxErr;
    int32_t EnDraughtFanPower;
    int32_t DFanPowerThreShold;
    int32_t EnDraughtFanTem;
    int32_t DFanTemThreShold;
} iems_lc_settings_t;

typedef struct {
    iems_ems_settings_t ems;
    iems_lc_settings_t lc;
    /* true only when both files were present and valid. false means "not both
     * loaded", NOT "nothing loaded": an OK file still fills its family while
     * the other one stays at defaults. */
    bool loaded;
} iems_pcs_settings_t;

/**
 * @brief Load ems_cfg.json and lc/lc_cfg.json from the injected source.
 *
 * Each file is OK (validated + applied), ABSENT (missing: defaults kept,
 * startup continues) or ERROR (bad content: -1, out untouched). A file is
 * applied atomically: parse into a fresh default struct first, so a failed
 * file never leaves half of its fields behind. `loaded` is true only when
 * both files loaded.
 *
 * @return 0 on success (including ABSENT files); -1 with "KIND path: detail"
 *         in err for bad content or unreadable files.
 */
int iems_pcs_settings_load(const iems_config_source_t *source, iems_pcs_settings_t *out,
                           char *err, size_t err_size);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_PCS_SETTINGS_H */
