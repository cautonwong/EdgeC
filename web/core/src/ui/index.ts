// Core UI 公开入口。Pack 只能通过 `@edgeos/web-core/ui` 使用这些组件（§6.6）。
import './tokens.css';

export { default as UiButton } from './components/Button.vue';
export { default as UiTextField } from './components/TextField.vue';
export { default as UiStatusBadge } from './components/StatusBadge.vue';
export { default as UiMetricCard } from './components/MetricCard.vue';
export { default as UiDataTable } from './components/DataTable.vue';
export { default as UiPermissionGate } from './components/PermissionGate.vue';
export { default as UiBanner } from './components/Banner.vue';
export type { Column } from './components/DataTable.vue';
