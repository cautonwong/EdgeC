/**
 * 实时通道 store（§6.3）。
 *
 * 职责只有三件：维护连接状态、缓存最近事件、把事件分发给订阅者。
 * 不在这里"根据事件改业务状态"——消费者自己决定收到消息后要不要重取，
 * 因为服务端不保留积压，事件本身不是唯一真相。
 */
import { defineStore } from 'pinia';
import { ref } from 'vue';
import type { LiveMessage, LiveState, LiveTopic } from '../../sdk/types';
import { createLiveChannel, type LiveChannel } from '../../sdk/live';

const TOPICS: LiveTopic[] = ['alarm', 'task', 'config-transaction', 'telemetry'];
const MAX_EVENTS = 50;

export const useLiveStore = defineStore('live', () => {
  const state = ref<LiveState>('idle');
  const lastSeq = ref(0);
  const lastEventAt = ref<string | null>(null);
  const events = ref<LiveMessage[]>([]);
  const resyncCount = ref(0);

  const listeners = new Set<(message: LiveMessage) => void>();
  let channel: LiveChannel | null = null;

  function start(): void {
    if (channel) return;
    channel = createLiveChannel({
      topics: TOPICS,
      onState: (next) => {
        state.value = next;
      },
      onMessage: (message) => {
        // seq 不连续说明漏了消息（服务端无积压）：让消费者整体重取，而不是拿事件当真相
        if (message.seq > 0 && lastSeq.value > 0 && message.seq > lastSeq.value + 1) {
          resyncCount.value += 1;
          for (const listener of listeners) listener({ ...message, type: 'resync' });
        }
        if (message.seq > lastSeq.value) lastSeq.value = message.seq;
        lastEventAt.value = message.ts;
        events.value = [message, ...events.value].slice(0, MAX_EVENTS);
        for (const listener of listeners) listener(message);
      },
      onResync: () => {
        resyncCount.value += 1;
        for (const listener of listeners) {
          listener({ type: 'resync', seq: lastSeq.value, ts: new Date().toISOString(), topic: 'system', payload: {} });
        }
      },
    });
  }

  function stop(): void {
    channel?.close();
    channel = null;
    state.value = 'closed';
  }

  /** 订阅回调，返回取消函数（组件卸载时必须调用，否则监听器泄漏）。 */
  function onEvent(listener: (message: LiveMessage) => void): () => void {
    listeners.add(listener);
    return () => listeners.delete(listener);
  }

  return { state, lastSeq, lastEventAt, events, resyncCount, start, stop, onEvent };
});
