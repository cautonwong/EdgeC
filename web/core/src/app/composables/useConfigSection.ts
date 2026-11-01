/**
 * 页面侧的配置编辑入口（modules → app 的唯一通道之一）。
 *
 * 为什么不让页面直接 import store：`tooling boundaries` 只允许页面依赖
 * `app/composables`，这样 Shell 的内部结构（store/router/views）可以改而不牵动页面。
 */
import { computed } from 'vue';
import { request } from '../../sdk/http';
import { useConfigTransactionStore } from '../stores/configTransaction';
import { useRegistryStore } from '../stores/registry';

export function useConfigSection(schemaId: string) {
  const store = useConfigTransactionStore();
  const registryStore = useRegistryStore();

  const schema = computed(() => registryStore.registry?.schemas.find((item) => item.id === schemaId) ?? null);

  function open(currentValues: Record<string, unknown>): boolean {
    if (!schema.value) return false;
    store.open(schema.value, currentValues);
    return true;
  }

  /** 整段配置页用这个：从设备读当前值 → 建立草稿 → 恢复未确认事务。 */
  async function load(): Promise<boolean> {
    if (!schema.value) return false;
    const section = await request<{ values: Record<string, unknown> }>(
      `/api/v1/config/sections/${encodeURIComponent(schemaId)}`,
    );
    store.open(schema.value, section.values ?? {});
    await store.resume();
    return true;
  }

  return { tx: store, schema, open, load };
}
