/**
 * Registry 与呈现视图（§4.4 / §6.2）。
 *
 * 运行期只做两件事：按当前身份/能力/许可**过滤**静态注册表，以及把不满足条件的
 * 页面解释清楚（403 / 不支持 / 未授权）。运行期不加载新代码、不新增路由。
 */
import { defineStore } from 'pinia';
import { computed, ref } from 'vue';
import type { PageDef, PageState, Registry } from '../../sdk/types';

function relativeLuminance(hex: string): number {
  const value = hex.replace('#', '');
  const channels = [0, 2, 4].map((offset) => parseInt(value.slice(offset, offset + 2), 16) / 255);
  const linear = (channel: number) =>
    channel <= 0.03928 ? channel / 12.92 : ((channel + 0.055) / 1.055) ** 2.4;
  return 0.2126 * linear(channels[0]) + 0.7152 * linear(channels[1]) + 0.0722 * linear(channels[2]);
}

/** Pick whichever of the two on-colour tokens reads better on the brand fill. */
function brandOnPrimary(brand: string): '#ffffff' | '#06121f' {
  const luminance = relativeLuminance(brand);
  const onWhite = 1.05 / (luminance + 0.05);
  const onDark = (luminance + 0.05) / (relativeLuminance('#06121f') + 0.05);
  return onWhite >= onDark ? '#ffffff' : '#06121f';
}
import {
  buildMenu,
  explainPage,
  findPageByPath,
  pageDenial,
  type VisibilityContext,
} from '../../sdk/visibility';
import { useCapabilityStore } from './capability';
import { useSessionStore } from './session';

export const useRegistryStore = defineStore('registry', () => {
  const registry = ref<Registry | null>(null);
  const session = useSessionStore();
  const capability = useCapabilityStore();

  const context = computed<VisibilityContext>(() => ({
    permissions: session.permissions,
    capabilities: capability.snapshot,
    capabilityUnknown: capability.unknown,
  }));

  const menu = computed(() => (registry.value ? buildMenu(registry.value, context.value) : []));
  const groups = computed(() => registry.value?.groups ?? []);

  function pageByPath(path: string): PageDef | undefined {
    return registry.value ? findPageByPath(registry.value, path) : undefined;
  }

  function stateOf(page: PageDef): PageState {
    return explainPage(page, context.value);
  }

  function denialOf(page: PageDef) {
    return pageDenial(page, context.value);
  }

  function setRegistry(next: Registry): void {
    registry.value = next;
    document.documentElement.dataset.brand = next.productId;
    document.documentElement.style.setProperty('--brand-primary', next.theme.brandColor);
    const hex = next.theme.brandColor.replace('#', '');
    document.documentElement.style.setProperty(
      '--brand-primary-rgb',
      [0, 2, 4].map((offset) => parseInt(hex.slice(offset, offset + 2), 16)).join(' '),
    );
    document.documentElement.style.setProperty('--brand-on-primary', brandOnPrimary(next.theme.brandColor));
  }

  return { registry, context, menu, groups, pageByPath, stateOf, denialOf, setRegistry };
});
