/**
 * Registry 的加载与自检。
 *
 * 摘要（digests.registry）由设备侧启动时校验，浏览器不做密码学校验——浏览器里
 * 没有同步 SHA-256，而"用 JS 现算一个"只会让人误以为前端是可信校验点。这里只做
 * 结构与版本兼容性检查，越界即抛错（fail-closed，绝不"猜一个默认值"）。
 */
import type { Registry } from './types';

export const CORE_SDK_VERSION = '1.6.0';

export function parseRegistry(raw: unknown, expectedProductId: string): Registry {
  if (!raw || typeof raw !== 'object') throw new Error('registry 不是对象');
  const registry = raw as Registry;
  if (registry.schemaVersion !== 1) throw new Error(`registry schemaVersion 不支持: ${registry.schemaVersion}`);
  if (registry.productId !== expectedProductId) {
    throw new Error(`registry 产品不匹配：期望 ${expectedProductId}，实际 ${registry.productId}`);
  }
  if (!Array.isArray(registry.pages) || !Array.isArray(registry.api) || !Array.isArray(registry.roles)) {
    throw new Error('registry 缺少 pages / api / roles');
  }
  if (!registry.digests?.registry) throw new Error('registry 缺少摘要，拒绝加载');
  if (registry.coreSdk.split('.')[0] !== CORE_SDK_VERSION.split('.')[0]) {
    throw new Error(`Core SDK 主版本不兼容：registry ${registry.coreSdk}，运行时 ${CORE_SDK_VERSION}`);
  }
  return registry;
}

/** 构建期产物与运行期常量必须一致，否则"以为在跑新代码"是最贵的排查成本。 */
export function assertSameCoreSdk(registry: Registry): void {
  if (registry.coreSdk !== CORE_SDK_VERSION) {
    console.warn(`[webui] registry 由 coreSdk ${registry.coreSdk} 生成，当前运行时为 ${CORE_SDK_VERSION}`);
  }
}

export function apiOperations(registry: Registry, method: string, path: string) {
  return registry.api.find((op) => op.method === method.toUpperCase() && op.path === path);
}
