/**
 * @file iems_pcs_settings.c
 * @brief P2-02a: strict loader for converted ems_cfg.json / lc/lc_cfg.json.
 *
 * Reuses the shared strict JSON boundary helpers from iems_config_loader.c
 * (private header) so the device/port and PCS trust boundaries stay identical.
 */

#include "iems_pcs_settings.h"

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "edge_log.h"
#include "iems_config_loader_priv.h"

/* ------------------------------------------------------------------ */
/* P2-02a: PCS/EMS settings (ems_cfg.json + lc/lc_cfg.json)            */
/* ------------------------------------------------------------------ */

#define IEMS_CONFIG_EMS_FILE "ems_cfg.json"
#define IEMS_CONFIG_LC_CFG_FILE "lc/lc_cfg.json"

/* 转换器（P1-03b）输出里只会出现非 x-deprecated 键；deprecated 旧名
 * pv_mode/cab_power/EnPcsRpComp/SetPcsPowerFactor/EnSocKeepMode 已被丢弃，
 * 这里按未知键拒绝，不信任上游。 */
static const char *const k_ems_keys[] = {
    "schema_version", "Pmlmax",         "K1",           "K2",
    "EnProtectDOD",   "EnBmsAlarm",     "SOCmax",       "SOCmin",
    "Pset",           "Adjpreriod",     "SleepMode",    "SocKeepActiveSOC",
    "SocKeepMargin",  "SocKeepChargePower", "PvMode",   "CabPower",
    "ENRPComp",       "SetPowerFactor", "SocKeepModeEnable", "tou",
};
static const char *const k_lc_keys[] = {
    "schema_version",      "EnBmsAlarm",        "EnProtectDOD",      "BmsPowerLimit",
    "PcsAutoTurn",         "PcsAutoTurnOffTime", "EnIoProtect",      "SOCmax",
    "SOCmin",              "PowerLimit",         "TMSCtrlTempCold",  "TMSCtrlTempHot",
    "TMSCtrlState",        "AirCondStartStopCtrl", "singleMaxVoltage", "singleMinVoltage",
    "enSingleVoltage",     "singleMaxVoltageErr", "singleMinVoltageErr", "SOCminErr",
    "SOCmaxErr",           "EnDraughtFanPower",  "DFanPowerThreShold", "EnDraughtFanTem",
    "DFanTemThreShold",
};
static const char *const k_tou_keys[] = {
    "StHour", "StMin", "EnHour", "EnMin", "Power", "RePower", "SOCUpperLimit", "SOCLowerLimit",
};

_Static_assert(sizeof(k_ems_keys) / sizeof(k_ems_keys[0]) == IEMS_EMS_SETTINGS_SCALAR_KEYS + 2,
               "ems keys must be schema_version + 18 scalars + tou");
_Static_assert(sizeof(k_lc_keys) / sizeof(k_lc_keys[0]) == IEMS_LC_SETTINGS_KEYS + 1,
               "lc keys must be schema_version + 24 fields");
_Static_assert(sizeof(k_tou_keys) / sizeof(k_tou_keys[0]) == IEMS_EMS_TOU_KEYS,
               "tou keys must match the schema");

static int _get_double(cJSON *object, const char *key, const char *path, char *err,
                       size_t err_size, double *out) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!item) return 0;
    char field[192];
    iems_config_path_key(field, sizeof(field), path, key);
    if (!cJSON_IsNumber(item)) {
        return iems_config_fail(err, err_size, "INVALID_TYPE", field, "expected number");
    }
    double value = item->valuedouble;
    if (!isfinite(value)) {
        return iems_config_fail(err, err_size, "INVALID_VALUE", field, "not finite");
    }
    if (out) *out = value;
    return 0;
}

static int _parse_ems_document(cJSON *root, const char *rel, iems_ems_settings_t *out, char *err,
                               size_t err_size) {
    if (iems_config_check_root(root, rel, err, err_size) != 0) return -1;
    if (iems_config_sensitive_scan(root, rel, err, err_size) != 0) return -1;
    if (iems_config_check_keys(root, rel, k_ems_keys, sizeof(k_ems_keys) / sizeof(k_ems_keys[0]), err,
                    err_size) != 0) {
        return -1;
    }

    iems_ems_settings_t s;
    memset(&s, 0, sizeof(s));
#define EMS_INT(field)                                                                       \
    if (iems_config_get_int(root, #field, 0, INT_MIN, rel, err, err_size, &s.field) != 0) return -1
    EMS_INT(Pmlmax);
    EMS_INT(K1);
    EMS_INT(K2);
    EMS_INT(EnProtectDOD);
    EMS_INT(EnBmsAlarm);
    EMS_INT(SOCmax);
    EMS_INT(SOCmin);
    EMS_INT(Pset);
    EMS_INT(Adjpreriod);
    EMS_INT(SleepMode);
    EMS_INT(PvMode);
    EMS_INT(CabPower);
    EMS_INT(ENRPComp);
    EMS_INT(SocKeepModeEnable);
#undef EMS_INT
#define EMS_DOUBLE(field)                                                                   \
    if (_get_double(root, #field, rel, err, err_size, &s.field) != 0) return -1
    EMS_DOUBLE(SocKeepActiveSOC);
    EMS_DOUBLE(SocKeepMargin);
    EMS_DOUBLE(SocKeepChargePower);
    EMS_DOUBLE(SetPowerFactor);
#undef EMS_DOUBLE

    cJSON *tou = cJSON_GetObjectItemCaseSensitive(root, "tou");
    if (tou) {
        char tou_path[192];
        iems_config_path_key(tou_path, sizeof(tou_path), rel, "tou");
        if (!cJSON_IsArray(tou)) {
            return iems_config_fail(err, err_size, "INVALID_TYPE", tou_path, "expected array");
        }
        if (cJSON_GetArraySize(tou) > (int)IEMS_EMS_TOU_MAX) {
            return iems_config_fail(err, err_size, "TOO_MANY_ITEMS", tou_path, "at most 6 entries");
        }
        size_t index = 0;
        cJSON *item;
        cJSON_ArrayForEach(item, tou) {
            char item_path[224];
            iems_config_path_index(item_path, sizeof(item_path), tou_path, index);
            if (!cJSON_IsObject(item)) {
                return iems_config_fail(err, err_size, "INVALID_TYPE", item_path, "expected object");
            }
            if (iems_config_check_keys(item, item_path, k_tou_keys, sizeof(k_tou_keys) / sizeof(k_tou_keys[0]),
                            err, err_size) != 0) {
                return -1;
            }
            iems_ems_tou_t *slot = &s.tou[index];
#define TOU_INT(field)                                                                       \
    if (iems_config_get_int(item, #field, 0, INT_MIN, item_path, err, err_size, &slot->field) != 0) return -1
            TOU_INT(StHour);
            TOU_INT(StMin);
            TOU_INT(EnHour);
            TOU_INT(EnMin);
            TOU_INT(Power);
            TOU_INT(RePower);
            TOU_INT(SOCUpperLimit);
            TOU_INT(SOCLowerLimit);
#undef TOU_INT
            index++;
        }
        s.tou_count = (uint32_t)index;
    }

    *out = s;
    return 0;
}

static int _parse_lc_document(cJSON *root, const char *rel, iems_lc_settings_t *out, char *err,
                              size_t err_size) {
    if (iems_config_check_root(root, rel, err, err_size) != 0) return -1;
    if (iems_config_sensitive_scan(root, rel, err, err_size) != 0) return -1;
    if (iems_config_check_keys(root, rel, k_lc_keys, sizeof(k_lc_keys) / sizeof(k_lc_keys[0]), err,
                    err_size) != 0) {
        return -1;
    }

    iems_lc_settings_t s;
    memset(&s, 0, sizeof(s));
#define LC_INT(field)                                                                        \
    if (iems_config_get_int(root, #field, 0, INT_MIN, rel, err, err_size, &s.field) != 0) return -1
    LC_INT(EnBmsAlarm);
    LC_INT(EnProtectDOD);
    LC_INT(BmsPowerLimit);
    LC_INT(PcsAutoTurn);
    LC_INT(PcsAutoTurnOffTime);
    LC_INT(EnIoProtect);
    LC_INT(SOCmax);
    LC_INT(SOCmin);
    LC_INT(PowerLimit);
    LC_INT(TMSCtrlTempCold);
    LC_INT(TMSCtrlTempHot);
    LC_INT(TMSCtrlState);
    LC_INT(AirCondStartStopCtrl);
    LC_INT(enSingleVoltage);
    LC_INT(SOCminErr);
    LC_INT(SOCmaxErr);
    LC_INT(EnDraughtFanPower);
    LC_INT(DFanPowerThreShold);
    LC_INT(EnDraughtFanTem);
    LC_INT(DFanTemThreShold);
#undef LC_INT
#define LC_DOUBLE(field)                                                                     \
    if (_get_double(root, #field, rel, err, err_size, &s.field) != 0) return -1
    LC_DOUBLE(singleMaxVoltage);
    LC_DOUBLE(singleMinVoltage);
    LC_DOUBLE(singleMaxVoltageErr);
    LC_DOUBLE(singleMinVoltageErr);
#undef LC_DOUBLE

    *out = s;
    return 0;
}

/* 0 = parsed; 1 = ABSENT; -1 = error (err already set). */
static int _read_pcs_document(const iems_config_source_t *source, const char *rel, cJSON **root_out,
                              char *err, size_t err_size) {
    char *data = NULL;
    size_t len = 0;
    iems_config_source_rc_t read_rc =
        source->read_file(source->ctx, rel, &data, &len, err, err_size);
    if (read_rc == IEMS_CONFIG_SOURCE_ABSENT) return 1;
    if (read_rc != IEMS_CONFIG_SOURCE_OK) {
        if (err && err[0]) return -1;
        return iems_config_fail(err, err_size, "READ_FAILED", rel, "cannot read file");
    }
    if (!data || data[len] != '\0') {
        free(data);
        return iems_config_fail(err, err_size, "INVALID_JSON", rel, "source did not NUL-terminate");
    }
    int parse_rc = iems_config_strict_parse(data, len, rel, root_out, err, err_size);
    free(data);
    return parse_rc == 0 ? 0 : -1;
}

int iems_pcs_settings_load(const iems_config_source_t *source, iems_pcs_settings_t *out, char *err,
                           size_t err_size) {
    if (err && err_size) err[0] = '\0';
    if (!source || !source->read_file || !out) {
        return iems_config_fail(err, err_size, "INVALID_ARGUMENT", "$", "source and output are required");
    }

    iems_pcs_settings_t result = *out;
    result.loaded = false;
    bool ems_ok = false;
    bool lc_ok = false;

    cJSON *root = NULL;
    int rc = _read_pcs_document(source, IEMS_CONFIG_EMS_FILE, &root, err, err_size);
    if (rc < 0) return -1;
    if (rc == 0) {
        iems_ems_settings_t ems;
        int parsed = _parse_ems_document(root, IEMS_CONFIG_EMS_FILE, &ems, err, err_size);
        cJSON_Delete(root);
        if (parsed != 0) return -1;
        result.ems = ems;
        ems_ok = true;
        LOG_INFO(LOG_CAT_CORE, "IEMS pcs settings: %s loaded", IEMS_CONFIG_EMS_FILE);
    } else {
        LOG_INFO(LOG_CAT_CORE, "IEMS pcs settings: %s absent; ems defaults kept",
                 IEMS_CONFIG_EMS_FILE);
    }

    root = NULL;
    rc = _read_pcs_document(source, IEMS_CONFIG_LC_CFG_FILE, &root, err, err_size);
    if (rc < 0) return -1;
    if (rc == 0) {
        iems_lc_settings_t lc;
        int parsed = _parse_lc_document(root, IEMS_CONFIG_LC_CFG_FILE, &lc, err, err_size);
        cJSON_Delete(root);
        if (parsed != 0) return -1;
        result.lc = lc;
        lc_ok = true;
        LOG_INFO(LOG_CAT_CORE, "IEMS pcs settings: %s loaded", IEMS_CONFIG_LC_CFG_FILE);
    } else {
        LOG_INFO(LOG_CAT_CORE, "IEMS pcs settings: %s absent; lc defaults kept",
                 IEMS_CONFIG_LC_CFG_FILE);
    }

    result.loaded = ems_ok && lc_ok;
    *out = result;
    return 0;
}
