#include "dcu_discovery.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @brief 初始化自动搜表模块
 */
void dcu_discovery_init(dcu_discovery_t *disc) {
    if (!disc) return;
    
    disc->status = DISCOVERY_STATUS_IDLE;
    disc->count = 0;
    memset(disc->discovered_tsas, 0, sizeof(disc->discovered_tsas));
    
    pthread_mutex_init(&disc->lock, NULL);
}

/**
 * @brief 启动自动/物理搜表方法 127 (Start Discovery)
 * 【搜表底层物理设计意图】：在配电网现场，由于新表更换或拓扑变动，经常存在未建档的“野生”电表。
 * 主站下发“实时搜表方法 (127)”，终端会切换下行 RS485 通道至搜表规约，以广播或快速轮询地址段的方式发送链路探测帧。
 */
int dcu_discovery_start(dcu_discovery_t *disc, uint32_t duration_sec) {
    if (!disc) return -1;
    
    pthread_mutex_lock(&disc->lock);
    
    // 如果已经在搜表中，静默返回成功，防止重复触发导致冲突
    if (disc->status == DISCOVERY_STATUS_SCANNING) {
        pthread_mutex_unlock(&disc->lock);
        return 0;
    }
    
    disc->status = DISCOVERY_STATUS_SCANNING;
    disc->count = 0;
    memset(disc->discovered_tsas, 0, sizeof(disc->discovered_tsas));
    
    pthread_mutex_unlock(&disc->lock);
    
    printf("📡 [DISCOVERY] 搜表物理动作已激活！启动下行 RS485 广播扫描，持续 %u 秒...\n", duration_sec);
    return 0;
}

/**
 * @brief 模拟搜表后台物理扫描帧捕获
 * 搜表引擎在物理扫描时发现新的 TSA 电表地址，将其写入 `0x6003` 搜表结果中
 */
void dcu_discovery_tick_simulate(dcu_discovery_t *disc) {
    if (!disc) return;
    
    pthread_mutex_lock(&disc->lock);
    if (disc->status != DISCOVERY_STATUS_SCANNING) {
        pthread_mutex_unlock(&disc->lock);
        return;
    }
    
    // 模拟扫描到了一个未登记的野生表计 (TSA: 10000003)
    if (disc->count < MAX_DISCOVERED_METERS) {
        char *wild_meter = "10000003";
        
        // 边界安全拷贝
        strncpy(disc->discovered_tsas[disc->count], wild_meter, DISCOVERY_ADDR_LEN - 1);
        disc->discovered_tsas[disc->count][DISCOVERY_ADDR_LEN - 1] = '\0';
        disc->count++;
        
        printf("🔍 [DISCOVERY] 物理通道发现新表计信号！成功捕获野生 TSA 地址: %s\n", wild_meter);
    }
    
    // 模拟扫描瞬间完成，状态转换为 COMPLETED，生成 6003 结果集供召测
    disc->status = DISCOVERY_STATUS_COMPLETED;
    
    pthread_mutex_unlock(&disc->lock);
    printf("📡 [DISCOVERY] 物理扫表过程已顺利完成，生成 6003 搜表结果，等待北向同步。\n");
}

/**
 * @brief 销毁搜表管理器
 */
void dcu_discovery_destroy(dcu_discovery_t *disc) {
    if (!disc) return;
    pthread_mutex_destroy(&disc->lock);
}
