/**
 * 配置事务的**纯**状态推导（不碰网络、不碰时钟）。
 *
 * 关键立场（§6.4）：倒计时只是显示，回滚责任在设备侧独立 watchdog。
 * 所以这里算出来的 `remainingMs` 只用于"还剩几秒"的文案与进度条；
 * 真正会不会回滚，以服务端返回的 state / deadline 为准。
 */
import type { ConfigTransaction, TransactionState } from './types';

export type Phase = 'idle' | 'dirty' | 'applying' | 'pending' | 'done' | 'failed';

export interface TransactionView {
  phase: Phase;
  /** 需要用户在页面上处理的提示（不处理就等着回滚） */
  needsAttention: boolean;
  remainingMs: number | null;
  remainingSeconds: number | null;
  expired: boolean;
  canConfirm: boolean;
  canRollback: boolean;
  label: string;
}

const LABELS: Record<TransactionState, string> = {
  DRAFT: '草稿未应用',
  VALIDATING: '校验中',
  APPLYING: '应用中',
  PENDING_CONFIRM: '已应用，待确认',
  CONFIRMED: '已确认',
  ROLLING_BACK: '回滚中',
  ROLLED_BACK: '已回滚',
  ROLLBACK_FAILED: '回滚失败（需要带外恢复）',
  FAILED: '应用失败',
};

export function describeState(state: TransactionState): string {
  return LABELS[state] ?? state;
}

/**
 * @param transaction 服务端返回的事务（可为 null：还没建草稿）
 * @param dirty       本地是否有未保存到草稿的修改
 * @param nowMs       当前时间（毫秒），由调用方传入以便单测
 */
export function viewTransaction(
  transaction: ConfigTransaction | null,
  dirty: boolean,
  nowMs: number,
): TransactionView {
  if (!transaction) {
    return {
      phase: dirty ? 'dirty' : 'idle',
      needsAttention: false,
      remainingMs: null,
      remainingSeconds: null,
      expired: false,
      canConfirm: false,
      canRollback: false,
      label: dirty ? '有未应用的修改' : '',
    };
  }

  const state = transaction.state;
  const deadlineMs = transaction.deadline ? Date.parse(transaction.deadline) : null;
  const remainingMs = deadlineMs === null ? null : Math.max(0, deadlineMs - nowMs);
  const expired = remainingMs !== null && remainingMs === 0 && state === 'PENDING_CONFIRM';

  const phase: Phase =
    state === 'PENDING_CONFIRM'
      ? 'pending'
      : state === 'APPLYING' || state === 'VALIDATING'
        ? 'applying'
        : state === 'CONFIRMED' || state === 'ROLLED_BACK'
          ? 'done'
          : state === 'FAILED' || state === 'ROLLBACK_FAILED'
            ? 'failed'
            : dirty
              ? 'dirty'
              : 'idle';

  return {
    phase,
    needsAttention: state === 'PENDING_CONFIRM' || state === 'ROLLBACK_FAILED',
    remainingMs,
    remainingSeconds: remainingMs === null ? null : Math.ceil(remainingMs / 1000),
    expired,
    canConfirm: state === 'PENDING_CONFIRM' && !expired,
    canRollback: state === 'PENDING_CONFIRM' || state === 'CONFIRMED',
    label: LABELS[state] ?? state,
  };
}

/** 倒计时文案：60 → "01:00"，9 → "00:09"。 */
export function formatRemaining(seconds: number | null): string {
  if (seconds === null) return '';
  const safe = Math.max(0, Math.floor(seconds));
  const mm = String(Math.floor(safe / 60)).padStart(2, '0');
  const ss = String(safe % 60).padStart(2, '0');
  return `${mm}:${ss}`;
}
