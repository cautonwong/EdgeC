/**
 * 实时通道客户端（§6.3）。
 *
 * 只做服务端 → 客户端的**只读**推送：连上后发 subscribe，断开自动重连（指数退避），
 * 重连成功时回调 `resync` —— 因为服务端不保留积压，漏掉的消息必须由消费者整体重取，
 * 而不是假装"接上了就连续"。
 *
 * 浏览器原生 WebSocket 足够，不引库；这里只负责连接生命周期与消息解码。
 */
import type { LiveMessage, LiveState } from './types';

export interface LiveChannelOptions {
  /** 相对路径，默认 /api/v1/ws */
  url?: string;
  topics: string[];
  onMessage: (message: LiveMessage) => void;
  onState?: (state: LiveState) => void;
  /** 重连成功后触发：消费者应整体重取一次（服务端无积压） */
  onResync?: () => void;
}

export interface LiveChannel {
  close: () => void;
  state: () => LiveState;
}

const BACKOFF_MS = [1000, 2000, 4000, 8000, 15000];

export function createLiveChannel(options: LiveChannelOptions): LiveChannel {
  let socket: WebSocket | null = null;
  let state: LiveState = 'idle';
  let attempt = 0;
  let closedByUs = false;
  let reconnectTimer: ReturnType<typeof setTimeout> | undefined;
  let hasConnectedBefore = false;

  const url = options.url ?? '/api/v1/ws';

  const setState = (next: LiveState) => {
    if (state === next) return;
    state = next;
    options.onState?.(next);
  };

  const send = (payload: unknown) => {
    if (socket && socket.readyState === WebSocket.OPEN) socket.send(JSON.stringify(payload));
  };

  const connect = () => {
    if (closedByUs) return;
    setState(attempt === 0 ? 'connecting' : 'reconnecting');
    socket = new WebSocket(url);

    socket.onopen = () => {
      attempt = 0;
      setState('open');
      for (const topic of options.topics) send({ type: 'subscribe', topic });
      if (hasConnectedBefore) options.onResync?.();
      hasConnectedBefore = true;
    };

    socket.onmessage = (event) => {
      try {
        const message = JSON.parse(String(event.data)) as LiveMessage;
        options.onMessage(message);
      } catch {
        // 无法解析的消息直接丢弃：实时通道不能因为一条脏数据把页面搞崩
      }
    };

    socket.onclose = () => {
      if (closedByUs) {
        setState('closed');
        return;
      }
      setState('reconnecting');
      const delay = BACKOFF_MS[Math.min(attempt, BACKOFF_MS.length - 1)];
      attempt += 1;
      reconnectTimer = setTimeout(connect, delay);
    };

    socket.onerror = () => {
      socket?.close();
    };
  };

  connect();

  return {
    close: () => {
      closedByUs = true;
      if (reconnectTimer) clearTimeout(reconnectTimer);
      socket?.close();
      setState('closed');
    },
    state: () => state,
  };
}
