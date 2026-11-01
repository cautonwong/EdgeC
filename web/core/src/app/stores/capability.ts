/**
 * 设备能力与许可（§4.5）。
 *
 * 三类信息必须分开：硬件能力 / 软件能力 / Entitlement。任何一个"未知"都按
 * **不支持**处理（fail-closed）——前端不能因为没拿到快照就假设功能可用。
 */
import { defineStore } from 'pinia';
import { computed, ref } from 'vue';
import type { CapabilitySnapshot } from '../../sdk/types';
import { request } from '../../sdk/http';

export const useCapabilityStore = defineStore('capability', () => {
  const snapshot = ref<CapabilitySnapshot | null>(null);
  const revision = ref<number | null>(null);
  const loading = ref(false);
  const error = ref<string | null>(null);

  const unknown = computed(() => snapshot.value === null);
  const entitlements = computed(() => snapshot.value?.entitlements ?? {});

  async function load(): Promise<void> {
    loading.value = true;
    error.value = null;
    try {
      const next = await request<CapabilitySnapshot>('/api/v1/system/capabilities');
      snapshot.value = next;
      revision.value = next.capabilityRevision;
    } catch (err) {
      // 能力不可用时保持 null：页面据此显示"当前设备不支持"，而不是猜一个默认值。
      snapshot.value = null;
      revision.value = null;
      error.value = err instanceof Error ? err.message : String(err);
    } finally {
      loading.value = false;
    }
  }

  /** capabilityRevision 变化意味着能力集合变了，前端能力视图必须重算（§4.5 规则 5）。 */
  async function refreshIfChanged(seenRevision: number): Promise<boolean> {
    await load();
    return revision.value !== seenRevision;
  }

  /** 登出/切换账号时清空：能力快照属于上一个会话，不能留给下一个主体当事实。 */
  function reset(): void {
    snapshot.value = null;
    revision.value = null;
    error.value = null;
  }

  return { snapshot, revision, loading, error, unknown, entitlements, load, refreshIfChanged, reset };
});
