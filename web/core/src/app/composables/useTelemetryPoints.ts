/**
 * 遥测点位列表（页面侧窄接口）。
 *
 * 数据面从"契约点位"迁到 `/api/v1/telemetry/current` 之后，需要点位的 id/name/unit
 * 来渲染选择器。这里只暴露"有哪些点位"，不暴露连接与推送细节。
 */
import { onMounted, ref } from 'vue';
import { request } from '../../sdk/http';

export interface TelemetryPointMeta {
  pointId: string;
  name: string;
  unit: string;
  group: string;
}

export function useTelemetryPoints() {
  const points = ref<TelemetryPointMeta[]>([]);
  const loading = ref(false);
  const error = ref('');

  async function load(): Promise<void> {
    loading.value = true;
    error.value = '';
    try {
      const result = await request<{ items: TelemetryPointMeta[] }>('/api/v1/telemetry/current');
      points.value = result.items.map((item) => ({
        pointId: item.pointId,
        name: item.name,
        unit: item.unit,
        group: item.group,
      }));
    } catch (err) {
      error.value = err instanceof Error ? err.message : String(err);
    } finally {
      loading.value = false;
    }
  }

  onMounted(load);

  return { points, loading, error, reload: load };
}
