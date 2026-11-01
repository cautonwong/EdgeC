// @edgeos/web-core 公开入口
//
// 依赖规则（由 `tooling boundaries` 强制）：
//   - Pack 的**页面**只能依赖 `./sdk`（类型 + 纯逻辑）与 `./ui`（组件 + Token）；
//   - Pack 的**入口 main.ts** 额外可以用 `./app` 的 `createProductApp`（组合根）；
//   - `./app` 内部实现（stores / router / views）不对外暴露，避免 Pack 伸手改 Shell。
export * from './sdk';
export * from './ui';

// ---- 迁移期兼容导出（deprecated）----
// 这些是旧「契约驱动 UI」时期的核心组件/页面。页面本身已按 Core 模块登记在
// core/modules.manifest.yaml 里，通过 Registry 装配；这里保留从包根导出的能力，
// 避免既有引用（产品入口、既有单测）在同一 PR 里被迫全部改写。
// 新代码请用 `@edgeos/web-core/ui`（L1/L2 组件）与 `@edgeos/web-core/sdk`（类型/纯逻辑）。
export { default as TagValue } from './components/TagValue.vue';
export { default as LiveTagCard } from './components/LiveTagCard.vue';
export { default as SankeyEnergyFlow } from './components/SankeyEnergyFlow.vue';
export { default as TOUArbitrageWidget } from './components/TOUArbitrageWidget.vue';
export { default as AntiReversePIDGauge } from './components/AntiReversePIDGauge.vue';
export { default as DemandPeakShavingWidget } from './components/DemandPeakShavingWidget.vue';
export { default as AICopilotDrawer } from './components/ai/AICopilotDrawer.vue';
export { default as McpToolCallCard } from './components/ai/McpToolCallCard.vue';
export { default as WebConsoleDrawer } from './components/console/WebConsoleDrawer.vue';
export { default as AnsiTerminalView } from './components/console/AnsiTerminalView.vue';
export { default as InteractiveShell } from './components/console/InteractiveShell.vue';
export { default as ProtocolSnifferView } from './components/console/ProtocolSnifferView.vue';
export { default as HexPayloadViewer } from './components/console/HexPayloadViewer.vue';
export { default as TwoManRuleModal } from './components/console/TwoManRuleModal.vue';
export { default as EnergyFlowView } from './modules/energy/EnergyFlowView.vue';
export { default as PacketCapture } from './modules/dcu/PacketCapture.vue';
export { default as PowerQuality } from './modules/dcu/PowerQuality.vue';
export { default as EventStreamHub } from './modules/events/EventStreamHub.vue';
export { default as LogStreamTerminal } from './modules/logs/LogStreamTerminal.vue';
export { default as MetricsMatrix } from './modules/metrics/MetricsMatrix.vue';
export { default as ChannelTopologyHeatmap } from './modules/devices/ChannelTopologyHeatmap.vue';
export { createProductApp, type ProductAppOptions, type ProductAppHandle } from './app/createProductApp';
export { useSessionStore } from './app/stores/session';
export { useCapabilityStore } from './app/stores/capability';
export { useRegistryStore } from './app/stores/registry';
