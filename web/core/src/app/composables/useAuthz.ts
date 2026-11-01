/**
 * Shell 提供给页面的**只读**授权上下文（单向依赖：页面 → shell，反向不允许）。
 *
 * 页面拿它来显示/隐藏按钮，仅此而已；每个写操作仍然由服务端逐请求判定。
 * 这条 import 路径是 `tooling boundaries` 唯一允许 modules 触碰 app 的地方。
 */
import { computed } from 'vue';
import { useCapabilityStore } from '../stores/capability';
import { useRegistryStore } from '../stores/registry';
import { useSessionStore } from '../stores/session';
import { capabilityValue } from '../../sdk/visibility';

export function useAuthz() {
  const session = useSessionStore();
  const capability = useCapabilityStore();
  const registryStore = useRegistryStore();

  const granted = computed(() => session.permissions);

  return {
    granted,
    username: computed(() => session.me?.username ?? ''),
    roles: computed(() => session.roles),
    /** allOf：全部满足 */
    can: (...ids: string[]) => ids.every((id) => granted.value.has(id)),
    /** anyOf：至少一条 */
    canAny: (...ids: string[]) => ids.some((id) => granted.value.has(id)),
    capability: (key: string) => capabilityValue(capability.snapshot, key),
    capabilitiesUnknown: computed(() => capability.unknown),
    registry: computed(() => registryStore.registry),
    systemInfo: computed(() => session.systemInfo),
  };
}
