// @edgeos/web-core 公开入口
//
// 依赖规则（由 `tooling boundaries` 强制）：
//   - Pack 的**页面**只能依赖 `./sdk`（类型 + 纯逻辑）与 `./ui`（组件 + Token）；
//   - Pack 的**入口 main.ts** 额外可以用 `./app` 的 `createProductApp`（组合根）；
//   - `./app` 内部实现（stores / router / views）不对外暴露，避免 Pack 伸手改 Shell。
export * from './sdk';
export * from './ui';
export { createProductApp, type ProductAppOptions, type ProductAppHandle } from './app/createProductApp';
export { useSessionStore } from './app/stores/session';
export { useCapabilityStore } from './app/stores/capability';
export { useRegistryStore } from './app/stores/registry';
