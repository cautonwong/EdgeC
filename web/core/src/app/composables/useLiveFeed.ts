/**
 * 页面订阅实时事件（modules → app 的窄接口）。
 *
 * 页面不该直接拿 store：`tooling boundaries` 只允许 modules 依赖 `app/composables` 与
 * `app/components`。这里把"订阅 + 卸载时取消"封装掉，页面只关心消息本身。
 */
import { computed, onBeforeUnmount, onMounted } from 'vue';
import type { LiveMessage } from '../../sdk/types';
import { useLiveStore } from '../stores/live';

export function useLiveFeed(handler: (message: LiveMessage) => void) {
  const live = useLiveStore();
  let unsubscribe: (() => void) | undefined;

  onMounted(() => {
    unsubscribe = live.onEvent(handler);
  });
  onBeforeUnmount(() => unsubscribe?.());

  return {
    state: computed(() => live.state),
    lastEventAt: computed(() => live.lastEventAt),
  };
}
