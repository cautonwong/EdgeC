// App Shell 的公开入口：只有产品入口（main.ts）需要它。
// 页面不要从这里 import —— 页面走 `@edgeos/web-core/sdk` / `/ui`，
// 需要的 shell 能力（授权上下文、配置事务）由 app/composables 提供。
export { createProductApp, type ProductAppOptions, type ProductAppHandle } from './createProductApp';
export { useSessionStore } from './stores/session';
export { useCapabilityStore } from './stores/capability';
export { useRegistryStore } from './stores/registry';
export { useConfigTransactionStore } from './stores/configTransaction';
export { useLiveStore } from './stores/live';
