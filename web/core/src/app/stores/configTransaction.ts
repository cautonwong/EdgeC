/**
 * 配置事务的前端编排（§6.4）。
 *
 * 一条铁律：**页面不是回滚的责任人**。倒计时只是显示，关掉浏览器也照样回滚——
 * 因为 deadline 与回滚由设备侧 watchdog 执行。所以这里的每个动作都是"告诉服务端"，
 * 本地只维护草稿与呈现状态。
 */
import { defineStore } from 'pinia';
import { computed, ref } from 'vue';
import type { ConfigSchema, ConfigTransaction } from '../../sdk/types';
import { ApiRequestError, request } from '../../sdk/http';
import { diffValues, validateValues, emptyValues } from '../../sdk/schema';
import { viewTransaction } from '../../sdk/transaction';

export const useConfigTransactionStore = defineStore('configTransaction', () => {
  const schema = ref<ConfigSchema | null>(null);
  const baseline = ref<Record<string, unknown>>({});
  const draft = ref<Record<string, unknown>>({});
  const transaction = ref<ConfigTransaction | null>(null);
  const fieldErrors = ref<Record<string, string>>({});
  const message = ref('');
  const error = ref('');
  const busy = ref(false);
  const nowMs = ref(Date.now());
  let ticker: ReturnType<typeof setInterval> | undefined;

  const dirty = computed(() => diffValues(baseline.value, draft.value).length > 0);
  const changes = computed(() => diffValues(baseline.value, draft.value));
  const view = computed(() => viewTransaction(transaction.value, dirty.value, nowMs.value));

  function startTicker(): void {
    if (ticker) return;
    ticker = setInterval(() => {
      nowMs.value = Date.now();
    }, 1000);
  }

  function stopTicker(): void {
    if (ticker) clearInterval(ticker);
    ticker = undefined;
  }

  /** 打开一个配置分区：schema 来自 Registry，baseline 是服务端当前值。 */
  function open(nextSchema: ConfigSchema, currentValues: Record<string, unknown>): void {
    schema.value = nextSchema;
    baseline.value = { ...currentValues };
    draft.value = { ...currentValues };
    transaction.value = null;
    fieldErrors.value = {};
    message.value = '';
    error.value = '';
    startTicker();
  }

  /** 刷新后恢复未完成事务：这是"页面关闭不影响事务"的可见部分。 */
  async function resume(): Promise<void> {
    if (!schema.value) return;
    try {
      const result = await request<{ items: ConfigTransaction[] }>(
        `/api/v1/config/transactions?sectionId=${encodeURIComponent(schema.value.id)}&state=PENDING_CONFIRM`,
      );
      const pending = result.items?.[0];
      if (pending) {
        transaction.value = pending;
        draft.value = { ...(pending.values as Record<string, unknown>) };
        message.value = '检测到未确认的配置事务，确认窗口仍在倒计时。';
      }
    } catch (err) {
      // 恢复失败不阻塞页面：只是没有"待确认"提示，用户仍可正常编辑
      error.value = describe(err);
    }
  }

  function setValue(field: string, value: unknown): void {
    draft.value = { ...draft.value, [field]: value };
    if (fieldErrors.value[field]) {
      const next = { ...fieldErrors.value };
      delete next[field];
      fieldErrors.value = next;
    }
  }

  function discard(): void {
    draft.value = { ...baseline.value };
    fieldErrors.value = {};
    message.value = '已丢弃本地修改';
    error.value = '';
  }

  function localValidate(): boolean {
    if (!schema.value) return false;
    const errors = validateValues(schema.value, draft.value);
    fieldErrors.value = Object.fromEntries(errors.map((item) => [item.field, item.message]));
    if (errors.length) {
      error.value = `有 ${errors.length} 项不符合 Schema，未提交`;
      return false;
    }
    return true;
  }

  /** 应用：没有草稿就先建，然后 apply。两步都在服务端留痕。 */
  async function apply(): Promise<void> {
    if (!schema.value || !localValidate()) return;
    busy.value = true;
    error.value = '';
    message.value = '';
    try {
      let tx = transaction.value;
      if (!tx || tx.state === 'CONFIRMED' || tx.state === 'ROLLED_BACK') {
        const created = await request<{ transactionId: string; state: ConfigTransaction['state'] }>(
          '/api/v1/config/transactions',
          { method: 'POST', body: { sectionId: schema.value.id, values: draft.value } },
        );
        tx = { ...(await getTransaction(created.transactionId)) };
      } else {
        await request(`/api/v1/config/transactions/${tx.transactionId}/sections/${schema.value.id}`, {
          method: 'PUT',
          body: { values: draft.value, baseRevision: tx.baseRevision },
        });
      }
      const applied = await request<ConfigTransaction>(`/api/v1/config/transactions/${tx.transactionId}/apply`, {
        method: 'POST',
        body: {},
        idempotencyKey: `${tx.transactionId}:apply`,
      });
      transaction.value = { ...applied };
      if (applied.state === 'PENDING_CONFIRM') {
        message.value = '已应用。请在倒计时结束前确认保留，否则设备会自动回滚。';
      } else if (applied.state === 'CONFIRMED') {
        baseline.value = { ...draft.value };
        message.value = '已应用并生效（该风险等级无需确认窗口）。';
      }
    } catch (err) {
      handleError(err);
    } finally {
      busy.value = false;
    }
  }

  async function confirm(): Promise<void> {
    if (!transaction.value) return;
    busy.value = true;
    error.value = '';
    try {
      const next = await request<ConfigTransaction>(
        `/api/v1/config/transactions/${transaction.value.transactionId}/confirm`,
        { method: 'POST', body: {}, idempotencyKey: `${transaction.value.transactionId}:confirm` },
      );
      transaction.value = { ...next };
      baseline.value = { ...draft.value };
      message.value = '已确认保留。';
    } catch (err) {
      handleError(err);
    } finally {
      busy.value = false;
    }
  }

  async function rollback(): Promise<void> {
    if (!transaction.value) return;
    busy.value = true;
    error.value = '';
    try {
      const next = await request<ConfigTransaction>(
        `/api/v1/config/transactions/${transaction.value.transactionId}/rollback`,
        { method: 'POST', body: {}, idempotencyKey: `${transaction.value.transactionId}:rollback` },
      );
      transaction.value = { ...next };
      draft.value = { ...baseline.value };
      message.value = '已回滚到应用前的配置。';
    } catch (err) {
      handleError(err);
    } finally {
      busy.value = false;
    }
  }

  /** 倒计时结束后状态可能已变成 ROLLED_BACK —— 让用户手动刷新，避免"页面说还在等"的假象。 */
  async function refresh(): Promise<void> {
    if (!transaction.value) return;
    try {
      transaction.value = { ...(await getTransaction(transaction.value.transactionId)) };
    } catch (err) {
      handleError(err);
    }
  }

  function reset(): void {
    stopTicker();
    schema.value = null;
    baseline.value = {};
    draft.value = {};
    transaction.value = null;
    fieldErrors.value = {};
    message.value = '';
    error.value = '';
  }

  function handleError(err: unknown): void {
    if (err instanceof ApiRequestError) {
      const fieldErrorsFromServer = (err.details as { fieldErrors?: Record<string, string> } | undefined)?.fieldErrors;
      if (fieldErrorsFromServer) fieldErrors.value = fieldErrorsFromServer;
      error.value = `${err.code}: ${err.message}`;
      return;
    }
    error.value = err instanceof Error ? err.message : String(err);
  }

  return {
    schema,
    baseline,
    draft,
    transaction,
    fieldErrors,
    message,
    error,
    busy,
    dirty,
    changes,
    view,
    open,
    resume,
    setValue,
    discard,
    apply,
    confirm,
    rollback,
    refresh,
    reset,
    startTicker,
    stopTicker,
  };
});

async function getTransaction(id: string): Promise<ConfigTransaction> {
  return request<ConfigTransaction>(`/api/v1/config/transactions/${id}`);
}

function describe(err: unknown): string {
  return err instanceof Error ? err.message : String(err);
}
