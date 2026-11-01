<script setup lang="ts">
/**
 * 配置事务操作条（§6.4 / §8.4）。
 *
 * 三件事必须让用户一眼看到：
 *   1. 有 N 项未应用的修改（不是"改完就算生效"）；
 *   2. 应用后还剩几秒可以确认（超时是**设备**回滚，不是页面）；
 *   3. 失败/回滚失败时给出可执行的下一步，而不是只报错。
 */
import { computed, onBeforeUnmount } from 'vue';
import { formatRemaining } from '../../sdk/transaction';
import { useConfigTransactionStore } from '../stores/configTransaction';
import UiBanner from '../../ui/components/Banner.vue';
import UiButton from '../../ui/components/Button.vue';

const store = useConfigTransactionStore();
const view = computed(() => store.view);
const showChanges = computed(() => store.changes.length > 0 && view.value.phase !== 'pending');

onBeforeUnmount(() => store.stopTicker());

function pretty(value: unknown): string {
  if (Array.isArray(value)) return value.length ? value.join(', ') : '（空）';
  if (value === undefined || value === null || value === '') return '（空）';
  return String(value);
}
</script>

<template>
  <div class="tx" data-testid="tx-bar">
    <UiBanner v-if="store.error" tone="danger" title="操作失败" data-testid="tx-error">{{ store.error }}</UiBanner>

    <UiBanner
      v-else-if="view.phase === 'pending'"
      :tone="view.expired ? 'danger' : 'warning'"
      :title="view.expired ? '确认窗口已结束' : '已应用，等待确认'"
      data-testid="tx-pending"
    >
      <span v-if="!view.expired">
        还剩 <strong data-testid="tx-countdown">{{ formatRemaining(view.remainingSeconds) }}</strong>，超时后设备会自动回滚到应用前的配置。
      </span>
      <span v-else>设备将按 watchdog 策略回滚。请刷新状态确认最终结果。</span>
      <template #actions>
        <UiButton
          v-if="view.canConfirm"
          variant="primary"
          size="sm"
          :loading="store.busy"
          data-testid="tx-confirm"
          @click="store.confirm()"
        >
          确认保留
        </UiButton>
        <UiButton variant="secondary" size="sm" :loading="store.busy" data-testid="tx-rollback" @click="store.rollback()">
          立即回滚
        </UiButton>
        <UiButton v-if="view.expired" variant="ghost" size="sm" data-testid="tx-refresh" @click="store.refresh()">
          刷新状态
        </UiButton>
      </template>
    </UiBanner>

    <UiBanner v-else-if="view.phase === 'applying'" tone="info" title="正在应用">请稍候，不要关闭页面。</UiBanner>

    <UiBanner v-else-if="view.phase === 'failed'" tone="danger" title="事务失败" data-testid="tx-failed">
      {{ view.label }}。请查看日志或联系管理员，必要时使用带外恢复。
    </UiBanner>

    <UiBanner v-else-if="view.phase === 'done' && store.message" tone="success" data-testid="tx-message">
      {{ store.message }}
    </UiBanner>

    <UiBanner v-else-if="view.phase === 'dirty'" tone="info" :title="`有 ${store.changes.length} 项未应用的修改`" data-testid="tx-dirty">
      这些修改还没有生效；应用后按风险等级可能需要确认。
      <template #actions>
        <UiButton variant="primary" size="sm" :loading="store.busy" data-testid="tx-apply" @click="store.apply()">
          应用
        </UiButton>
        <UiButton variant="secondary" size="sm" data-testid="tx-discard" @click="store.discard()">丢弃</UiButton>
      </template>
    </UiBanner>

    <UiBanner v-if="store.message && view.phase !== 'done' && view.phase !== 'pending'" tone="info" data-testid="tx-hint">
      {{ store.message }}
    </UiBanner>

    <details v-if="showChanges" class="tx__changes" data-testid="tx-changes">
      <summary>查看差异（{{ store.changes.length }} 项）</summary>
      <table>
        <thead>
          <tr>
            <th>字段</th>
            <th>当前值</th>
            <th>修改为</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="change in store.changes" :key="change.field">
            <td class="mono">{{ change.field }}</td>
            <td>{{ pretty(change.before) }}</td>
            <td>{{ pretty(change.after) }}</td>
          </tr>
        </tbody>
      </table>
    </details>
  </div>
</template>

<style scoped>
.tx {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
  margin-bottom: var(--space-4);
}
.tx__changes {
  border: 1px solid var(--border-default);
  border-radius: var(--radius-md);
  padding: var(--space-2) var(--space-3);
  background: var(--bg-surface);
  font-size: var(--font-size-sm);
}
.tx__changes summary {
  cursor: pointer;
  color: var(--text-secondary);
}
.tx__changes table {
  width: 100%;
  border-collapse: collapse;
  margin-top: var(--space-2);
}
.tx__changes th,
.tx__changes td {
  text-align: left;
  padding: var(--space-1) var(--space-2);
  border-bottom: 1px solid var(--border-default);
}
.mono {
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
}
</style>
