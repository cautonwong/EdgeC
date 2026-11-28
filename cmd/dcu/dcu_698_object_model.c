#include <stdint.h>

#include "dcu_comm_info.h"
#include "dcu_698_object_internal.h"
#include "dcu_698_param.h"
#include "dcu_698_var.h"
#include "edge_slave_plane.h"

#define BIT(id) ((uint8_t)(1U << ((id)&7)))
#define BYTE(id) ((id) >> 3)

static dcu_698_object_t g_objects[] = {
    {.oi = OI_METER_ARCHIVE,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_6000_execute,
     .method_mask = {[BYTE(127)] = BIT(127),
                     [BYTE(128)] = BIT(128) | BIT(129) | BIT(130) | BIT(131) | BIT(132) | BIT(133) |
                                   BIT(134)}},

    {.oi = OI_ARCHIVE_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

/* 类 8 白名单：对象行只广告属性 2；4000 的 A3/A4 由参数模块接管。 */
#define PARAM_ROW(v)             \
    {.oi = (v),                  \
     .class_id = IC_PARAM,       \
     .scope = DCU_OBJ_SINGLETON, \
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    DCU_698_PARAM_OIS(PARAM_ROW)
#undef PARAM_ROW

        {.oi = OI_TASK_SCHEDULER,
         .class_id = IC_ACQUISITION,
         .scope = DCU_OBJ_SHARED,
         .attr_mask = {[BYTE(2)] = BIT(2)},
         .method = dcu_698_acq_6012_execute,
         .method_mask = {[BYTE(1)] = BIT(1),
                         [BYTE(127)] = BIT(127),
                         [BYTE(128)] = BIT(128) | BIT(129) | BIT(130)}},

    /* 6013 实例由 6012 A2 的 task_id 寻址；A2 由 resolve_axdr 编码，无标量读数。 */
    {.oi = OI_TASK_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

    {.oi = OI_NORMAL_SCHEME,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_6014_execute,
     .method_mask = {[BYTE(127)] = BIT(127),
                     [BYTE(128)] = BIT(128) | BIT(129) | BIT(130)}},

    /* 6015 实例由 6014 A2 的方案编号寻址；A2 由 resolve_axdr 编码。 */
    {.oi = OI_NORMAL_SCHEME_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

    {.oi = OI_EVENT_SCHEME,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_6016_execute,
     .method_mask = {[BYTE(127)] = BIT(127),
                     [BYTE(128)] = BIT(128) | BIT(129) | BIT(130)}},
    {.oi = OI_EVENT_SCHEME_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_TRANSPARENT_SCHEME,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_6018_execute,
     .method_mask = {[BYTE(127)] = BIT(127),
                     [BYTE(128)] = BIT(128) | BIT(129) | BIT(130) | BIT(131)}},
    {.oi = OI_TRANSPARENT_SCHEME_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_TRANSPARENT_RESULT,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_TRANSPARENT_RESULT_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_REPORT_SCHEME,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_601C_execute,
     .method_mask = {[BYTE(127)] = BIT(127), [BYTE(128)] = BIT(128) | BIT(129)}},
    {.oi = OI_REPORT_SCHEME_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_RULE_LIBRARY,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_601E_execute,
     .method_mask = {[BYTE(127)] = BIT(127), [BYTE(128)] = BIT(128) | BIT(129)}},
    {.oi = OI_RULE,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_ACQ_STATUS_SET,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_ACQ_STATUS,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_TASK_MONITOR_SET,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_TASK_MONITOR,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_ACQ_START_TIME,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_ACQ_SUCCESS_TIME,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_ACQ_STORE_TIME,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

    {.oi = (uint16_t)((OAD_ACTIVE_ENERGY >> 16) & 0xFFFF),
     .class_id = IC_ENERGY,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(5)}},
    {.oi = (uint16_t)((OAD_REACTIVE_ENERGY >> 16) & 0xFFFF),
     .class_id = IC_ENERGY,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(5)}},
    {.oi = (uint16_t)((OAD_VOLTAGE >> 16) & 0xFFFF),
     .class_id = IC_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_CURRENT >> 16) & 0xFFFF),
     .class_id = IC_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_ACTIVE_POWER_TOTAL >> 16) & 0xFFFF),
     .class_id = IC_POWER,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_POWER_FACTOR_TOTAL >> 16) & 0xFFFF),
     .class_id = IC_POWER,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_GRID_FREQUENCY >> 16) & 0xFFFF),
     .class_id = IC_DATA_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},

    {.oi = 0x1010, .class_id = IC_MAX_DEMAND, .scope = DCU_OBJ_PER_METER},
    {.oi = 0x1011, .class_id = IC_MAX_DEMAND, .scope = DCU_OBJ_PER_METER},
    {.oi = 0x1012, .class_id = IC_MAX_DEMAND, .scope = DCU_OBJ_PER_METER},
    {.oi = 0x1013, .class_id = IC_MAX_DEMAND, .scope = DCU_OBJ_PER_METER},
    {.oi = 0x200D, .class_id = IC_HARMONIC, .scope = DCU_OBJ_PER_METER},
    {.oi = 0x200E, .class_id = IC_HARMONIC, .scope = DCU_OBJ_PER_METER},

    /* 2100–2104/2110–2114/2120–2124：Class 14/15/16 没有统计生产者，空掩码
     * 让 resolver 按类定义如实回 DAR 5，不伪造统计值。 */
    {.oi = 0x2100, .class_id = IC_INTERVAL_STAT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2101, .class_id = IC_INTERVAL_STAT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2102, .class_id = IC_INTERVAL_STAT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2103, .class_id = IC_INTERVAL_STAT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2104, .class_id = IC_INTERVAL_STAT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2110, .class_id = IC_AVERAGE, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2111, .class_id = IC_AVERAGE, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2112, .class_id = IC_AVERAGE, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2113, .class_id = IC_AVERAGE, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2114, .class_id = IC_AVERAGE, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2120, .class_id = IC_EXTREMUM, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2121, .class_id = IC_EXTREMUM, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2122, .class_id = IC_EXTREMUM, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2123, .class_id = IC_EXTREMUM, .scope = DCU_OBJ_SHARED},
    {.oi = 0x2124, .class_id = IC_EXTREMUM, .scope = DCU_OBJ_SHARED},

    /* 2401：Class 12 脉冲计量没有脉冲量来源，空掩码让 resolver 按类定义
     * 如实回 DAR 5，不返回空数组或伪造脉冲量。 */
    {.oi = OI_PULSE_METER, .class_id = IC_PULSE, .scope = DCU_OBJ_SINGLETON},

    /* 2301–2308：Class 23 总加组。产品没有总加配置/总加值/滑差/功控来源，空掩码
     * 让 resolver 按类定义如实回 DAR 5；总加与功控方法没有处理器，绝不执行。 */
    {.oi = OI_GROUP_FIRST, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_GROUP_FIRST + 1, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_GROUP_FIRST + 2, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_GROUP_FIRST + 3, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_GROUP_FIRST + 4, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_GROUP_FIRST + 5, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_GROUP_FIRST + 6, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_GROUP_LAST, .class_id = IC_GROUP, .scope = DCU_OBJ_SINGLETON},

    /* F000/F001/F002：Class 18 文件传输。产品没有文件存储/DFU 激活/校验/回滚来源，
     * 空掩码让 resolver 按类定义如实回 DAR 5；传输/校验/上传/下载方法没有处理器，
     * 绝不接收分块、不校验、不激活、不回滚。 */
    {.oi = OI_FILE_FRAME, .class_id = IC_FILE_TRANSFER, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_FILE_CHUNK, .class_id = IC_FILE_TRANSFER, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_FILE_EXT, .class_id = IC_FILE_TRANSFER, .scope = DCU_OBJ_SINGLETON},

    /* 4300/4307–4309：Class 19 设备管理。产品没有设备描述/版本/复位/初始化端口，
     * 空掩码让 resolver 按类定义如实回 DAR 5；复位与初始化方法没有处理器，
     * 绝不真正重启或清数据。 */
    {.oi = OI_DEVICE_ELECTRIC, .class_id = IC_DEVICE_MGMT, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_DEVICE_WATER, .class_id = IC_DEVICE_MGMT, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_DEVICE_GAS, .class_id = IC_DEVICE_MGMT, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_DEVICE_HEAT, .class_id = IC_DEVICE_MGMT, .scope = DCU_OBJ_SINGLETON},

    /* 4500/4501 与 4510–4517：Class 25/26 通信模块。产品没有 APN/ICCID/IMSI/
     * 信号/端口配置来源；平台只给第一个以太网口的 MAC，所以只广告 4510 A5，
     * 其余空掩码让 resolver 按类定义如实回 DAR 5，不伪造通信参数。 */
    {.oi = OI_WIRELESS_COMM_FIRST, .class_id = IC_WIRELESS_PUBLIC, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_WIRELESS_COMM_LAST, .class_id = IC_WIRELESS_PUBLIC, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_ETHERNET_COMM_FIRST,
     .class_id = IC_ETHERNET,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(5)] = BIT(5)}},
    {.oi = OI_ETHERNET_COMM_FIRST + 1, .class_id = IC_ETHERNET, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_ETHERNET_COMM_FIRST + 2, .class_id = IC_ETHERNET, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_ETHERNET_COMM_FIRST + 3, .class_id = IC_ETHERNET, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_ETHERNET_COMM_FIRST + 4, .class_id = IC_ETHERNET, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_ETHERNET_COMM_FIRST + 5, .class_id = IC_ETHERNET, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_ETHERNET_COMM_FIRST + 6, .class_id = IC_ETHERNET, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_ETHERNET_COMM_LAST, .class_id = IC_ETHERNET, .scope = DCU_OBJ_SINGLETON},

    /* F300/F301：Class 17 显示没有显示配置来源，空掩码让 resolver 按类定义
     * 如实回 DAR 5，不伪造轮显表或显示参数。 */
    {.oi = 0xF300, .class_id = IC_DISPLAY, .scope = DCU_OBJ_SINGLETON},
    {.oi = 0xF301, .class_id = IC_DISPLAY, .scope = DCU_OBJ_SINGLETON},

    /* 5004 只走 GetRecord（D1-07）：A2 是冻结数据数组而不是条数，把条数当属性值
     * 就是假语义。属性与方法一律不暴露，类内定义交给 class_defines_* 决定 DAR。 */
    {.oi = 0x5000,
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = 0x5001,
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = 0x5002,
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = 0x5003,
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = (uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF),
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = 0x5005,
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = 0x5006,
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = 0x5007,
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method = dcu_698_freeze_execute,
     .method_mask = {[BYTE(3)] = BIT(3) | BIT(4) | BIT(5) | BIT(7), [BYTE(8)] = BIT(8)}},
    {.oi = 0x5008, .class_id = IC_RECORD, .scope = DCU_OBJ_SHARED},
    {.oi = 0x5009, .class_id = IC_RECORD, .scope = DCU_OBJ_SHARED},
    {.oi = 0x500A, .class_id = IC_RECORD, .scope = DCU_OBJ_SHARED},
    {.oi = 0x500B, .class_id = IC_RECORD, .scope = DCU_OBJ_SHARED},
    {.oi = 0x5011, .class_id = IC_RECORD, .scope = DCU_OBJ_SHARED},

    /* A.9：8000 的 A2/A4/A5 是配置/告警/命令状态；产品没有继电器输出通道，
     * GET 走 resolve 统一回 DAR 2，不伪造状态位。 */
    {.oi = OI_REMOTE_CONTROL,
     .class_id = IC_CONTROL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(4) | BIT(5)},
     .method = dcu_698_control_execute,
     .method_mask = {[BYTE(DCU_ACTION_METHOD_FIRST)] = BIT(DCU_ACTION_METHOD_FIRST),
                     [BYTE(DCU_ACTION_METHOD_FIRST + 1)] =
                         BIT(DCU_ACTION_METHOD_FIRST + 1) | BIT(DCU_ACTION_METHOD_FIRST + 2) |
                         BIT(DCU_ACTION_METHOD_FIRST + 3) | BIT(DCU_ACTION_METHOD_FIRST + 4)}},

    {.oi = OI_POWER_SAVE,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(5)}},
    {.oi = OI_TERMINAL_SECURITY,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_POWER_CTRL_PERIOD,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_POWER_CTRL_ALARM,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_DAILY_POWER_CTRL,
     .class_id = IC_CONTROL,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    /* A.9 列出 810E/810F/8110，但没有定义属性/方法：只保留对象存在性。 */
    {.oi = OI_CONTROL_OBJECT, .class_id = IC_PARAM, .scope = DCU_OBJ_SHARED},
    {.oi = OI_TRIP_ROUND, .class_id = IC_PARAM, .scope = DCU_OBJ_SHARED},
    {.oi = OI_ELEC_CTRL_VALUE, .class_id = IC_PARAM, .scope = DCU_OBJ_SHARED},

    /* F100：Class 21 ESAM。产品没有 ESAM 硬件/会话来源，软件 ESAM 模拟只留 C API；
     * 698 面空掩码，GET 属性与方法按类定义如实回 DAR 5，不伪造序列号/会话/MAC。 */
    {.oi = OI_ESAM_SECURITY, .class_id = IC_ESAM, .scope = DCU_OBJ_SINGLETON},

    /* A.12 的 Class 22 只读属性没有产品来源：如实广告，resolver 回 DAR 2。 */
    {.oi = OI_CHANNEL_CONFIG,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_RS485_PORT,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_INFRARED_PORT,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_DIGITAL_INPUT,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(4)}},
    {.oi = OI_DC_ANALOG,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(4)}},
    {.oi = OI_RELAY_OUTPUT,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_ALARM_OUTPUT,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(4)}},
    {.oi = OI_MULTI_TERMINAL,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_AC_SAMPLING,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_LOCAL_MODULE,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(5) | BIT(6)}},
    {.oi = OI_PULSE_INPUT,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_BLUETOOTH,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},
    {.oi = OI_WIRELESS_PRIVATE,
     .class_id = IC_PERIPHERAL,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(4) | BIT(5)}},

    {.oi = OI_METER_DISCOVERY,
     .class_id = IC_COLLECTION,
     .scope = DCU_OBJ_SINGLETON,
     .read = dcu_698_acq_6002_read,
     .method = dcu_698_acq_6002_execute,
     /* A2 结果数组、A3 当前元素个数、A6 记录数、A10 空闲/搜表中。 */
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(6), [BYTE(10)] = BIT(10)},
     .method_mask = {[BYTE(127)] = BIT(127), [BYTE(128)] = BIT(128) | BIT(129)}},

    {.oi = OI_FILE_SET, .class_id = IC_COLLECTION, .scope = DCU_OBJ_SINGLETON},
    {.oi = OI_SCRIPT_SET, .class_id = IC_COLLECTION, .scope = DCU_OBJ_SINGLETON},

    /* 3009/300C/3011/3013：Class 7 事件对象骨架。产品没有事件引擎/生产者，空掩码
     * 让 resolver 按类定义如实回 DAR 5，不伪造事件记录或状态。 */
    {.oi = 0x3009, .class_id = IC_EVENT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x300C, .class_id = IC_EVENT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x3011, .class_id = IC_EVENT, .scope = DCU_OBJ_SHARED},
    {.oi = 0x3013, .class_id = IC_EVENT, .scope = DCU_OBJ_SHARED},

    /* 300B：Class 24 无功需量超限事件，标准定义 A5..A9。产品没有该事件引擎，空掩码
     * 且不挂实例，resolver 按类定义如实回 DAR 5，不伪造事件记录或计数。 */
    {.oi = 0x300B, .class_id = IC_SUB_EVENT, .scope = DCU_OBJ_SHARED},

    {.oi = OI_VOLTAGE_DROP,
     .class_id = IC_SUB_EVENT,
     .scope = DCU_OBJ_SHARED,
     .read = dcu_698_event_read,
     .write = dcu_698_event_write,
     .method = dcu_698_event_execute,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(6),
                   [BYTE(11)] = BIT(11) | BIT(12) | BIT(14)},
     .method_mask = {[BYTE(1)] = BIT(1)}},
    {.oi = OI_OVER_CURRENT,
     .class_id = IC_SUB_EVENT,
     .scope = DCU_OBJ_SHARED,
     .read = dcu_698_event_read,
     .write = dcu_698_event_write,
     .method = dcu_698_event_execute,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(6),
                   [BYTE(11)] = BIT(11) | BIT(12) | BIT(14)},
     .method_mask = {[BYTE(1)] = BIT(1)}},
};

#define OBJECT_COUNT (sizeof(g_objects) / sizeof(g_objects[0]))

static bool g_initialized = false;
static router_bus_ops_t g_router_bus_ops;
static router_sink_ops_t g_router_sink_ops;
static dcu_inventory_t *s_router_inv = NULL;
static dcu_control_sink_t s_control_sink;
static void *s_control_ctx;

static dcu_698_object_t *_find_mut(uint16_t oi) {
    for (size_t i = 0; i < OBJECT_COUNT; i++) {
        if (g_objects[i].oi == oi) return &g_objects[i];
    }
    return NULL;
}

static void _set_instance(uint16_t oi, const void *instance) {
    dcu_698_object_t *obj = _find_mut(oi);
    if (obj) obj->instance = instance;
}

int dcu_698_object_model_init(dcu_inventory_t *inv, dcu_poller_t *poller,
                              router_sink_ops_t sink_ops, dcu_esam_t *esam,
                              dcu_discovery_t *discovery, dcu_event_obj_t *evt_3000,
                              dcu_event_obj_t *evt_3005, router_bus_ops_t bus_ops) {
    g_router_sink_ops = sink_ops;
    g_router_bus_ops = bus_ops;
    s_router_inv = inv;

    for (size_t i = 0; i < OBJECT_COUNT; i++) g_objects[i].instance = NULL;
    g_initialized = true;

    dcu_comm_info_t comm_info;
    dcu_comm_info_read(&comm_info);
    dcu_comm_info_install(&comm_info);

    _set_instance(OI_METER_ARCHIVE, inv);
    _set_instance(OI_ARCHIVE_UNIT, inv);
    _set_instance(OI_TASK_SCHEDULER, poller);
    _set_instance(OI_NORMAL_SCHEME, poller);
    _set_instance(OI_ESAM_SECURITY, esam);
    _set_instance(OI_METER_DISCOVERY, discovery);
    _set_instance(OI_VOLTAGE_DROP, evt_3000);
    _set_instance(OI_OVER_CURRENT, evt_3005);
    return 0;
}

void dcu_698_object_model_reset(void) {
    for (size_t i = 0; i < OBJECT_COUNT; i++) g_objects[i].instance = NULL;
    g_initialized = false;
    g_router_bus_ops = (router_bus_ops_t){0};
    g_router_sink_ops = (router_sink_ops_t){0};
    s_router_inv = NULL;
    dcu_comm_info_reset();
    dcu_698_param_reset();
}

void dcu_698_object_model_set_control_sink(dcu_control_sink_t fn, void *ctx) {
    s_control_sink = fn;
    s_control_ctx = ctx;
}

const dcu_698_object_t *dcu_698_find(uint16_t oi) {
    if (!g_initialized) return NULL;
    return _find_mut(oi);
}

size_t dcu_698_object_count(void) {
    return OBJECT_COUNT;
}

const dcu_698_object_t *dcu_698_object_at(size_t index) {
    return index < OBJECT_COUNT ? &g_objects[index] : NULL;
}

/* 附录 A 的类定义上限。 */
static const struct {
    uint8_t class_id;
    uint8_t max_attr;
} k_class_max_attr[] = {
    {1, 5},   {2, 3},  {3, 3},   {4, 3},  {5, 6},   {6, 3},   {7, 10},  {8, 2},  {9, 3},
    {10, 3},  {11, 4}, {12, 19}, {13, 5}, {14, 3},  {15, 3},  {16, 3},  {17, 4}, {18, 3},
    {19, 10}, {20, 5}, {21, 15}, {22, 4}, {23, 18}, {24, 14}, {25, 11}, {26, 5},
};

bool dcu_698_class_defines_attr(uint8_t class_id, uint8_t attr_id) {
    if (attr_id == 0) return false;
    for (size_t i = 0; i < sizeof(k_class_max_attr) / sizeof(k_class_max_attr[0]); i++) {
        if (k_class_max_attr[i].class_id == class_id)
            return attr_id <= k_class_max_attr[i].max_attr;
    }
    return false;
}

/* 附录 A 的实例属性上限：7.3 类表没有这些号，但它们仍由标准为该 OI 定义。
 * 6002 的 A5..A10 在表 A.7，不在 7.3.11；F209/F20C 的 A5/A6 在表 A.13；
 * F000 的 A4/A5、F001/F002 的 A4 在表 A.11；4000 的 A3/A4、8001 的 A3..A5 在表 A.5。 */
static const struct {
    uint16_t oi;
    uint8_t max_attr;
} k_oi_max_attr[] = {
    {0x4000, 4},
    {OI_POWER_SAVE, 5},
    {OI_METER_DISCOVERY, 10},
    {OI_LOCAL_MODULE, 6},
    {OI_WIRELESS_PRIVATE, 5},
    {OI_FILE_FRAME, 5},
    {OI_FILE_CHUNK, 4},
    {OI_FILE_EXT, 4},
};

bool dcu_698_oi_defines_attr(uint16_t oi, uint8_t class_id, uint8_t attr_id) {
    if (dcu_698_class_defines_attr(class_id, attr_id)) return true;
    if (attr_id == 0) return false;
    for (size_t i = 0; i < sizeof(k_oi_max_attr) / sizeof(k_oi_max_attr[0]); i++) {
        if (k_oi_max_attr[i].oi == oi) return attr_id <= k_oi_max_attr[i].max_attr;
    }
    return false;
}

/* 附录 A 各接口类自身方法号的上限；absent_methods 的 bit n 标记 7.3 类表缺号（OCI 专属的 127+ 方法另算）。 */
static const struct {
    uint8_t class_id;
    uint8_t max_method;
    uint8_t absent_methods;
} k_class_max_method[] = {
    {1, 2, 0},  {2, 2, 0},  {3, 2, 0},   {4, 2, 0},  {5, 2, 0},   {6, 2, 0},
    {7, 5, 1u << 3}, {8, 2, 0}, {9, 8, 1u << 6},
    {10, 3, 0}, {11, 2, 0}, {12, 4, 0},  {13, 7, 0}, {14, 4, 0},  {15, 4, 0},
    {16, 4, 0}, {17, 6, 0}, {18, 8, 0},
    {19, 6, 0}, {20, 2, 0}, {21, 12, 0}, {22, 2, 0}, {23, 5, 0},
    {24, 5, 1u << 3}, {25, 1, 0}, {26, 1, 0},
};

/* A.12/A.8 与 Class 19 子设备的 OI 专属方法（127 起）；类表只到各类自身的方法号。 */
static const struct {
    uint16_t oi;
    uint8_t max_method;
} k_oi_max_method[] = {
    {OI_CHANNEL_CONFIG, 127}, {OI_RS485_PORT, 127},    {OI_INFRARED_PORT, 127},
    {OI_RELAY_OUTPUT, 127},   {OI_MULTI_TERMINAL, 127}, {OI_LOCAL_MODULE, 128},
    {OI_BLUETOOTH, 127},      {OI_FILE_SET, 129},       {OI_SCRIPT_SET, 130},
    {OI_DEVICE_WATER, 129},   {OI_DEVICE_GAS, 129},     {OI_DEVICE_HEAT, 129},
};

bool dcu_698_class_defines_method(uint8_t class_id, uint8_t method_id) {
    if (method_id == 0) return false;
    for (size_t i = 0; i < sizeof(k_class_max_method) / sizeof(k_class_max_method[0]); i++) {
        if (k_class_max_method[i].class_id == class_id) {
            return method_id <= k_class_max_method[i].max_method &&
                   !(k_class_max_method[i].absent_methods & (1u << method_id));
        }
    }
    return false;
}

static bool _class_defines_method(uint8_t class_id, uint16_t oi, uint8_t method_id) {
    if (method_id == 0) return false;
    if (oi == OI_REMOTE_CONTROL && method_id >= DCU_ACTION_METHOD_FIRST &&
        method_id <= DCU_ACTION_METHOD_LAST) {
        return true;
    }
    if (oi == OI_METER_ARCHIVE && method_id >= 127 && method_id <= 134) return true;
    if (oi == OI_METER_DISCOVERY && method_id >= 127 && method_id <= 129) return true;
    if (oi == OI_TASK_SCHEDULER && method_id >= 127 && method_id <= 130) return true;
    /* 表A.11：F001 分块传输另有方法 9（读文件）、10（软件比对），号段在 127 以下。 */
    if (oi == OI_FILE_CHUNK && method_id >= 9 && method_id <= 10) return true;
    if (method_id >= 127) {
        for (size_t i = 0; i < sizeof(k_oi_max_method) / sizeof(k_oi_max_method[0]); i++) {
            if (k_oi_max_method[i].oi == oi) return method_id <= k_oi_max_method[i].max_method;
        }
    }
    return dcu_698_class_defines_method(class_id, method_id);
}

const router_sink_ops_t *dcu_698_internal_sink_ops(void) {
    return &g_router_sink_ops;
}

dcu_inventory_t *dcu_698_internal_inventory(void) {
    return s_router_inv;
}

void dcu_698_internal_control_sink(dcu_control_sink_t *out_fn, void **out_ctx) {
    if (out_fn) *out_fn = s_control_sink;
    if (out_ctx) *out_ctx = s_control_ctx;
}

const router_bus_ops_t *dcu_698_object_model_bus_ops(void) {
    return &g_router_bus_ops;
}

int dcu_698_read(uint32_t oad, void *out_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj || !obj->read) return -1;
    if (!dcu_mask_test(obj->attr_mask, attr_id)) return -1;
    return obj->read(obj, attr_id, elem_idx, out_val);
}

int dcu_698_write(uint32_t oad, const void *in_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj || !obj->write) return -1;
    if (!dcu_mask_test(obj->attr_mask, attr_id)) return -1;
    return obj->write(obj, attr_id, elem_idx, in_val);
}

int dcu_698_check_call(uint32_t omd, const dcu_698_object_t **obj_out) {
    if (obj_out) *obj_out = NULL;
    uint16_t oi = (uint16_t)((omd >> 16) & 0xFFFF);
    uint8_t method_id = (uint8_t)((omd >> 8) & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!obj->method || !dcu_mask_test(obj->method_mask, method_id)) {
        return _class_defines_method(obj->class_id, obj->oi, method_id)
                   ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                   : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (obj_out) *obj_out = obj;
    return 0;
}

int dcu_698_call(uint32_t omd, const void *param, void *result) {
    uint8_t method_id = (uint8_t)((omd >> 8) & 0xFF);
    const dcu_698_object_t *obj = NULL;
    int rc = dcu_698_check_call(omd, &obj);
    if (rc != 0) return rc;
    return obj->method(obj, method_id, param, result);
}

int dcu_698_snapshot(uint16_t oi, uint32_t device_id, void *out_msgs, int max_count) {
    uint8_t *p_out = (uint8_t *)out_msgs;
    if (!g_router_bus_ops.read_shadow || !p_out || max_count <= 0) return 0;

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj) return 0;

    int captured = 0;

    /* class_id 是标准接口类（3xxx 为 24）；快照按处理器族分派。 */
    if (obj->read == dcu_698_event_read) {
        dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
        if (!evt) return 0;
        for (int i = 0; i < evt->snapshot_count && captured < max_count; i++) {
            uint32_t oad = evt->snapshot_oads[i];
            void *dest = p_out + (size_t)captured * g_router_bus_ops.msg_size;
            if (g_router_bus_ops.read_shadow(g_router_bus_ops.ctx, oad, device_id, dest) == 0) {
                captured++;
            }
        }
    }

    return captured;
}
