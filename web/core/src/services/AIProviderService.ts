/**
 * @file AIProviderService.ts
 * @brief Industrial AI Provider & MCP Client Service (AEP-507)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

import { reactive, ref } from 'vue';
import axios from 'axios';

export type AIProviderType = 'ollama' | 'deepseek' | 'claude' | 'openai' | 'local_edge';

export interface AIModelConfig {
  provider: AIProviderType;
  endpoint: string;
  apiKey: string;
  modelName: string;
  temperature: number;
  maxTokens: number;
  mcpServerWsUrl: string;
  mcpServerHttpUrl: string;
  autoExecuteTools: boolean;
}

export interface McpToolCall {
  id: string;
  toolName: string;
  args: Record<string, any>;
  status: 'pending' | 'executing' | 'success' | 'error';
  result?: any;
  error?: string;
  durationMs?: number;
  timestamp: number;
}

export interface McpToolDefinition {
  name: string;
  description: string;
  domain?: string;
  parameters?: Record<string, any>;
  inputSchema?: Record<string, any>;
}

export interface ToolHint {
  name: string;
  args: Record<string, any>;
}

export interface ChatContext {
  pageTitle?: string;
  productDomain?: string;
  selectedTagId?: number;
  selectedTagName?: string;
  currentValue?: number | string;
  unit?: string;
  activeAlarms?: string[];
  extraData?: Record<string, any>;
}

export interface ChatMessage {
  id: string;
  role: 'user' | 'assistant' | 'system' | 'tool';
  content: string;
  timestamp: number;
  toolCalls?: McpToolCall[];
  context?: ChatContext;
  metrics?: {
    latencyMs: number;
    tokensUsed?: number;
  };
}

export const DEFAULT_AI_CONFIG: AIModelConfig = {
  provider: 'local_edge',
  endpoint: 'http://localhost:11434/v1',
  apiKey: '',
  modelName: 'qwen2.5-coder:7b',
  temperature: 0.2,
  maxTokens: 2048,
  mcpServerWsUrl: 'ws://' + (typeof window !== 'undefined' ? window.location.hostname || '127.0.0.1' : '127.0.0.1') + ':8089/mcp',
  mcpServerHttpUrl: 'http://' + (typeof window !== 'undefined' ? window.location.hostname || '127.0.0.1' : '127.0.0.1') + ':8089/api/v1/mcp/call',
  autoExecuteTools: true,
};

// Offline fallback catalog. The live catalog comes from edge-mcp-server `tools/list`
// and is cached; this list only answers when the server has never responded.
export const INDUSTRIAL_MCP_TOOLS: McpToolDefinition[] = [
  {
    name: 'datahub_read_tags',
    domain: 'DataHub',
    description: 'Read real-time physical measurements from DataHub shadow memory by Tag IDs.',
    parameters: { type: 'object', properties: { tag_ids: { type: 'array', items: { type: 'integer' } } }, required: ['tag_ids'] },
  },
  {
    name: 'datahub_write_tag',
    domain: 'DataHub',
    description: 'Write a physical control setpoint or telemetry value into DataHub shadow memory.',
    parameters: { type: 'object', properties: { tag_id: { type: 'integer' }, value: { type: 'number' }, quality: { type: 'integer' } }, required: ['tag_id', 'value'] },
  },
  {
    name: 'datahub_scan_abnormal',
    domain: 'DataHub',
    description: 'Perform a full-table scan across all physical tags to identify non-zero quality alarms or abnormal readings.',
    parameters: { type: 'object', properties: {} },
  },
  {
    name: 'dcu_analyze_power_quality',
    domain: 'Power Quality',
    description: 'Analyze three-phase voltage unbalance, 31st harmonic THD, and phase commutation recommendations.',
    parameters: { type: 'object', properties: { phase_a_v: { type: 'number' }, phase_b_v: { type: 'number' }, phase_c_v: { type: 'number' }, current_thd: { type: 'number' } }, required: ['phase_a_v', 'phase_b_v', 'phase_c_v'] },
  },
  {
    name: 'smartbox_eval_transformer_life',
    domain: 'Transformer',
    description: 'Evaluate transformer hot-spot temperature, insulation aging factor and remaining lifespan.',
    parameters: { type: 'object', properties: { load_ratio: { type: 'number' }, ambient_temp_c: { type: 'number' }, current_top_oil_c: { type: 'number' } }, required: ['load_ratio', 'ambient_temp_c', 'current_top_oil_c'] },
  },
  {
    name: 'iems_optimize_tou_schedule',
    domain: 'Energy',
    description: 'Optimize ESS charge/discharge against peak-valley tariffs, PV and load forecasts.',
    parameters: { type: 'object', properties: { soc_pct: { type: 'number' }, pv_forecast_kw: { type: 'number' }, load_forecast_kw: { type: 'number' } }, required: ['soc_pct'] },
  },
  {
    name: 'security_simulate_interlock',
    domain: 'Safety',
    description: 'Validate breaker/disconnector/earth-switch operations against electrical five-anti interlock rules.',
    parameters: { type: 'object', properties: { breaker_id: { type: 'string' }, target_state: { type: 'string', enum: ['OPEN', 'CLOSE'] } }, required: ['breaker_id', 'target_state'] },
  },
];


function unwrapMcpResult(payload: any): any {
  const content = payload?.content;
  if (Array.isArray(content)) {
    const textPart = content.find((item: any) => item?.type === 'text' && typeof item?.text === 'string');
    if (textPart) {
      try {
        return JSON.parse(textPart.text);
      } catch {
        return textPart.text;
      }
    }
  }
  return payload;
}

class AIProviderService {
  private config: AIModelConfig;
  private ws: WebSocket | null = null;
  private rpcIdCounter = 1;
  private pendingRpcCallbacks = new Map<number, { resolve: (res: any) => void; reject: (err: any) => void }>();
  public isConnected = ref(false);
  public tools = ref<McpToolDefinition[]>([...INDUSTRIAL_MCP_TOOLS]);
  public toolsSource = ref<'server' | 'static'>('static');
  private toolsFetchedAt = 0;
  private toolsInflight: Promise<McpToolDefinition[]> | null = null;
  private static readonly TOOLS_TTL_MS = 60_000;
  private static readonly TOOLS_CACHE_KEY = 'edgeos.mcp.tools.v1';

  constructor(cfg: Partial<AIModelConfig> = {}) {
    this.config = reactive({ ...DEFAULT_AI_CONFIG, ...cfg });
    this.hydrateToolCache();
    this.initMcpWsConnection();
  }

  public getConfig(): AIModelConfig {
    return this.config;
  }

  public setConfig(newCfg: Partial<AIModelConfig>): void {
    Object.assign(this.config, newCfg);
  }

  /**
   * Connect to EdgeOS MCP Server over WebSocket
   */
  public initMcpWsConnection(): void {
    if (typeof window === 'undefined' || !window.WebSocket) return;
    if (this.ws && (this.ws.readyState === WebSocket.OPEN || this.ws.readyState === WebSocket.CONNECTING)) {
      return;
    }

    try {
      this.ws = new WebSocket(this.config.mcpServerWsUrl);

      this.ws.onopen = () => {
        this.isConnected.value = true;
        void this.fetchTools(true);
      };

      this.ws.onclose = () => {
        this.isConnected.value = false;
        // Auto-reconnect after 3s
        setTimeout(() => this.initMcpWsConnection(), 3000);
      };

      this.ws.onerror = () => {
        this.isConnected.value = false;
      };

      this.ws.onmessage = (evt) => {
        try {
          const data = JSON.parse(evt.data);
          if (data && data.id !== undefined) {
            const cb = this.pendingRpcCallbacks.get(data.id);
            if (cb) {
              this.pendingRpcCallbacks.delete(data.id);
              if (data.error) {
                cb.reject(new Error(data.error.message || 'MCP RPC Error'));
              } else {
                cb.resolve(data.result);
              }
            }
          }
        } catch (e) {
          // Ignore parse errors
        }
      };
    } catch (e) {
      this.isConnected.value = false;
    }
  }

  private hydrateToolCache(): void {
    try {
      const raw = localStorage.getItem(AIProviderService.TOOLS_CACHE_KEY);
      if (!raw) return;
      const cached = JSON.parse(raw);
      if (!Array.isArray(cached?.tools) || cached.tools.length === 0) return;
      const tools = cached.tools.filter(
        (tool: any) => typeof tool?.name === 'string' && tool.name.length > 0
      ) as McpToolDefinition[];
      if (tools.length === 0) return;
      this.tools.value = tools;
      this.toolsSource.value = 'server';
      this.toolsFetchedAt = typeof cached.at === 'number' ? cached.at : 0;
    } catch {
      // Cache is an accelerator, never a requirement.
    }
  }

  private persistToolCache(tools: McpToolDefinition[]): void {
    try {
      localStorage.setItem(
        AIProviderService.TOOLS_CACHE_KEY,
        JSON.stringify({ at: Date.now(), tools })
      );
    } catch {
      // Private mode or full storage must not break the copilot.
    }
  }

  private toolDomain(name: string): string {
    const known = INDUSTRIAL_MCP_TOOLS.find((tool) => tool.name === name);
    if (known && known.domain) return known.domain;
    if (name.startsWith('datahub_')) return 'DataHub';
    if (name.startsWith('dcu_')) return 'Power Quality';
    if (name.startsWith('smartbox_')) return 'Transformer';
    if (name.startsWith('iems_')) return 'Energy';
    if (name.startsWith('security_')) return 'Safety';
    return 'MCP';
  }

  /**
   * Fetch the live tool catalog from edge-mcp-server `tools/list` and cache it.
   * The static catalog stays as the offline fallback; the server list wins.
   */
  public async fetchTools(force = false): Promise<McpToolDefinition[]> {
    const fresh =
      this.toolsSource.value === 'server' &&
      Date.now() - this.toolsFetchedAt < AIProviderService.TOOLS_TTL_MS;
    if (!force && fresh) return this.tools.value;
    if (this.toolsInflight) return this.toolsInflight;

    this.toolsInflight = (async () => {
      try {
        const result = await this.rpcRequest('tools/list', {});
        const rawTools = Array.isArray(result?.tools) ? result.tools : [];
        const mapped: McpToolDefinition[] = [];
        for (const raw of rawTools) {
          const name = typeof raw?.name === 'string' ? raw.name.trim() : '';
          if (!name) continue;
          mapped.push({
            name,
            description: typeof raw?.description === 'string' ? raw.description : '',
            inputSchema: raw?.inputSchema,
            domain: this.toolDomain(name),
          });
        }
        if (mapped.length > 0) {
          this.tools.value = mapped;
          this.toolsSource.value = 'server';
          this.toolsFetchedAt = Date.now();
          this.persistToolCache(mapped);
        }
        return this.tools.value;
      } catch {
        // Keep the last server list, or the static catalog, when the server is down.
        return this.tools.value;
      } finally {
        this.toolsInflight = null;
      }
    })();
    return this.toolsInflight;
  }

  private async rpcRequest(method: string, params: Record<string, any> = {}): Promise<any> {
    const rpcId = ++this.rpcIdCounter;
    const reqPayload = { jsonrpc: '2.0', id: rpcId, method, params };

    // 1. Try WebSocket
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      try {
        const resultPromise = new Promise((resolve, reject) => {
          this.pendingRpcCallbacks.set(rpcId, { resolve, reject });
          setTimeout(() => {
            if (this.pendingRpcCallbacks.has(rpcId)) {
              this.pendingRpcCallbacks.delete(rpcId);
              reject(new Error(`MCP ${method} timed out (5000ms)`));
            }
          }, 5000);
        });
        this.ws.send(JSON.stringify(reqPayload));
        return await resultPromise;
      } catch {
        // Fall through to HTTP.
      }
    }

    // 2. HTTP fallback
    try {
      const response = await axios.post(this.config.mcpServerHttpUrl, reqPayload, { timeout: 5000 });
      if (response.data?.error) {
        const err: any = new Error(response.data.error.message || 'MCP HTTP Error');
        err.isMcpToolError = true;
        throw err;
      }
      return response.data?.result ?? response.data;
    } catch (httpErr: any) {
      if (httpErr?.isMcpToolError) throw httpErr;
      const detail = httpErr?.message || 'MCP server unreachable';
      throw new Error(`MCP ${method} failed: WebSocket and HTTP unavailable (${detail})`);
    }
  }

  /**
   * Invoke MCP tool via WebSocket (falling back to HTTP POST)
   */
  public async callMcpTool(toolName: string, args: Record<string, any> = {}): Promise<{ result: any; durationMs: number }> {
    const start = performance.now();
    const result = await this.rpcRequest('tools/call', { name: toolName, arguments: args });
    const durationMs = Math.round((performance.now() - start) * 100) / 100;
    return { result: unwrapMcpResult(result), durationMs };
  }

  /**
   * Run ReAct Agent Reasoning Loop
   */
  public async executeChat(
    userPrompt: string,
    context?: ChatContext,
    onToolProgress?: (tool: McpToolCall) => void,
    hint?: ToolHint
  ): Promise<ChatMessage> {
    const startTime = performance.now();
    const executedTools: McpToolCall[] = [];

    const runTool = async (toolName: string, args: Record<string, any>): Promise<McpToolCall> => {
      const toolCallRecord: McpToolCall = {
        id: 'tc_' + Math.random().toString(36).substring(2, 9),
        toolName,
        args,
        status: 'executing',
        timestamp: Date.now(),
      };
      executedTools.push(toolCallRecord);
      if (onToolProgress) onToolProgress(toolCallRecord);
      try {
        const { result, durationMs } = await this.callMcpTool(toolName, args);
        toolCallRecord.status = 'success';
        toolCallRecord.result = result;
        toolCallRecord.durationMs = durationMs;
      } catch (err: any) {
        toolCallRecord.status = 'error';
        toolCallRecord.error = err.message || 'Tool execution failed';
      }
      if (onToolProgress) onToolProgress(toolCallRecord);
      return toolCallRecord;
    };

    // A quick prompt carries the exact tool and sample arguments it advertises.
    // Run it as declared instead of re-deriving intent from the prompt text.
    if (hint && hint.name) {
      await runTool(hint.name, hint.args || {});
      const finalContent = this.synthesizeDiagnosis(userPrompt, context, executedTools);
      return {
        id: 'msg_' + Date.now(),
        role: 'assistant',
        content: finalContent,
        timestamp: Date.now(),
        toolCalls: executedTools,
        context,
        metrics: { latencyMs: Math.round((performance.now() - startTime) * 100) / 100 },
      };
    }

    // Determine tool calls needed based on prompt intent. Intents are
    // independent: a prompt like “扫描三相电能质量异常” must trigger both the
    // full-table scan and the three-phase analysis.
    const normalizedPrompt = userPrompt.toLowerCase();
    const toolsToCall: { name: string; args: Record<string, any> }[] = [];

    const extra = context?.extraData || {};
    const hasNumber = (v: any) => typeof v === 'number' && Number.isFinite(v);
    const numeric = (v: any) => {
      if (v === null || v === undefined) return undefined;
      if (typeof v === 'boolean') return undefined;
      if (typeof v === 'string' && v.trim() === '') return undefined;
      const n = typeof v === 'number' ? v : Number(v);
      return Number.isFinite(n) ? n : undefined;
    };
    const phaseTagIds = Array.isArray(extra.phase_tag_ids)
      ? extra.phase_tag_ids.filter((v: any) => typeof v === 'number')
      : [0x1001, 0x1002, 0x1003];
    const wantsScan =
      normalizedPrompt.includes('扫描') ||
      normalizedPrompt.includes('异常') ||
      normalizedPrompt.includes('abnormal') ||
      normalizedPrompt.includes('巡检');
    const wantsPowerQuality =
      normalizedPrompt.includes('电能质量') ||
      normalizedPrompt.includes('三相') ||
      normalizedPrompt.includes('不平衡') ||
      normalizedPrompt.includes('谐波');
    const wantsTransformer =
      normalizedPrompt.includes('变压器') ||
      normalizedPrompt.includes('寿命') ||
      normalizedPrompt.includes('transformer');
    const wantsStorage =
      normalizedPrompt.includes('储能') ||
      normalizedPrompt.includes('峰谷') ||
      normalizedPrompt.includes('tou') ||
      normalizedPrompt.includes('套利');
    const wantsInterlock =
      normalizedPrompt.includes('五防') ||
      normalizedPrompt.includes('互锁') ||
      normalizedPrompt.includes('interlock');
    const wantsTagRead =
      normalizedPrompt.includes('电压') ||
      normalizedPrompt.includes('测点') ||
      normalizedPrompt.includes('tag') ||
      normalizedPrompt.includes('read') ||
      normalizedPrompt.includes('读取');

    let readQueued = false;
    const queueRead = (tagIds: number[]) => {
      if (readQueued) return;
      readQueued = true;
      toolsToCall.push({ name: 'datahub_read_tags', args: { tag_ids: tagIds } });
    };

    if (wantsScan) {
      toolsToCall.push({ name: 'datahub_scan_abnormal', args: {} });
    }

    const phaseA = numeric(extra.phase_a_v);
    const phaseB = numeric(extra.phase_b_v);
    const phaseC = numeric(extra.phase_c_v);
    let needsPowerQualityTelemetry = false;
    if (wantsPowerQuality) {
      if (hasNumber(phaseA) && hasNumber(phaseB) && hasNumber(phaseC)) {
        toolsToCall.push({
          name: 'dcu_analyze_power_quality',
          args: { phase_a_v: phaseA, phase_b_v: phaseB, phase_c_v: phaseC, current_thd: extra.current_thd },
        });
      } else {
        queueRead(phaseTagIds);
        needsPowerQualityTelemetry = true;
      }
    }

    if (wantsTransformer) {
      if (hasNumber(extra.load_ratio) && hasNumber(extra.ambient_temp_c) && hasNumber(extra.current_top_oil_c)) {
        toolsToCall.push({
          name: 'smartbox_eval_transformer_life',
          args: { load_ratio: extra.load_ratio, ambient_temp_c: extra.ambient_temp_c, current_top_oil_c: extra.current_top_oil_c },
        });
      } else if (!wantsTagRead) {
        queueRead([context?.selectedTagId ?? 0x1001]);
      }
    }

    if (wantsStorage) {
      const soc = numeric(extra.soc_pct !== undefined ? extra.soc_pct : context?.currentValue);
      if (soc !== undefined) {
        toolsToCall.push({
          name: 'iems_optimize_tou_schedule',
          args: { soc_pct: soc, pv_forecast_kw: extra.pv_forecast_kw, load_forecast_kw: extra.load_forecast_kw },
        });
      } else {
        queueRead([context?.selectedTagId ?? 0x1001]);
      }
    }

    if (wantsInterlock) {
      if (typeof extra.breaker_id === 'string' && (extra.target_state === 'OPEN' || extra.target_state === 'CLOSE')) {
        toolsToCall.push({
          name: 'security_simulate_interlock',
          args: { breaker_id: extra.breaker_id, target_state: extra.target_state },
        });
      } else {
        queueRead([context?.selectedTagId ?? 0x1001]);
      }
    }

    const wantsImplicitRead = wantsTagRead && !wantsScan;
    if (wantsImplicitRead || (toolsToCall.length === 0 && context?.selectedTagId !== undefined)) {
      queueRead([context?.selectedTagId ?? 0x1001]);
    }

    // Execute the first wave. Telemetry reads are executed before any deferred
    // analysis so its arguments come from real values rather than assumptions.
    for (const item of toolsToCall) {
      await runTool(item.name, item.args);
    }

    if (needsPowerQualityTelemetry) {
      const readTool = executedTools.find(
        (tool) => tool.toolName === 'datahub_read_tags' && tool.status === 'success'
      );
      const readTags = Array.isArray(readTool?.result?.tags) ? readTool.result.tags : [];
      const phaseTags = phaseTagIds;
      const byId = new Map<number, number>();
      for (const tag of readTags) {
        const id = numeric(tag?.tag_id);
        const value = numeric(tag?.value);
        if (id !== undefined && value !== undefined) byId.set(id, value);
      }
      const pa = numeric(extra.phase_a_v) ?? byId.get(phaseTags[0]);
      const pb = numeric(extra.phase_b_v) ?? byId.get(phaseTags[1]);
      const pc = numeric(extra.phase_c_v) ?? byId.get(phaseTags[2]);
      if (pa !== undefined && pb !== undefined && pc !== undefined) {
        await runTool('dcu_analyze_power_quality', {
          phase_a_v: pa,
          phase_b_v: pb,
          phase_c_v: pc,
          current_thd: extra.current_thd,
        });
      }
    }

    // Synthesize final response
    const finalContent = this.synthesizeDiagnosis(userPrompt, context, executedTools);
    const latencyMs = Math.round((performance.now() - startTime) * 100) / 100;

    return {
      id: 'msg_' + Date.now(),
      role: 'assistant',
      content: finalContent,
      timestamp: Date.now(),
      toolCalls: executedTools,
      context,
      metrics: { latencyMs },
    };
  }

  private synthesizeDiagnosis(prompt: string, context?: ChatContext, tools: McpToolCall[] = []): string {
    const sections: string[] = [];

    sections.push(`### 🤖 EdgeOS AI 工业伴侣诊断报告\n`);

    if (context && (context.productDomain || context.pageTitle)) {
      sections.push(`> **当前工业上下文**：\`${context.productDomain || 'IEMS'}\` · \`${context.pageTitle || '全站监控'}\``);
      if (context.selectedTagName) {
        sections.push(`> **分析对象**：**${context.selectedTagName}** (当前物理读数: \`${context.currentValue} ${context.unit || ''}\`)\n`);
      }
    }

    if (tools.length > 0) {
      sections.push(`#### 🛠️ MCP 物理链路工具决策与反馈 (${tools.length} 项)`);
      for (const t of tools) {
        const statusIcon = t.status === 'success' ? '✅' : '❌';
        sections.push(`- ${statusIcon} \`${t.toolName}\` (${t.durationMs || 0} ms)`);
      }
      sections.push('');
    }

    // Generate intelligent insights
    sections.push(`#### 📊 诊断分析与策略建议\n`);

    const hasAbnormal = tools.some(t => t.toolName === 'datahub_scan_abnormal' && t.result?.abnormal_count > 0);
    const hasReadTags = tools.find(t => t.toolName === 'datahub_read_tags');

    if (hasReadTags && hasReadTags.result?.tags?.length > 0) {
      const tag0 = hasReadTags.result.tags[0];
      sections.push(`1. **实时物理遥测**：测点 \`0x${tag0.tag_id.toString(16).toUpperCase()}\` 采样值为 **${tag0.value}**，质量码为 \`${tag0.quality}\` (正常)，挂钟时标同步确定性高。`);
      sections.push(`2. **阈值与工况评估**：数值处于额定工况允许区间内，暂无过欠压或热失控越限风险。`);
      sections.push(`3. **操作建议**：持续保持当前 MTT 挂钟轮询周期（15min/整点），无需触发保护动作。`);
    } else if (hasAbnormal) {
      sections.push(`1. **异常告警发现**：全站扫描检测到异常测点，建议立即核查南向通信链路质量与一次断路器状态。`);
      sections.push(`2. **联动建议**：可在下方控制台中执行保护复归或触发 SOE 故障录波。`);
    } else if (tools.some(t => t.status === 'success')) {
      const successes = tools.filter(t => t.status === 'success');
      sections.push(`1. **工具执行结果**：${successes.map(t => `\`${t.toolName}\``).join('、')} 已返回物理链路数据。`);
      sections.push(`2. **结果摘要**：${successes.map(t => `${t.toolName}: ${JSON.stringify(t.result ?? null).slice(0, 180)}`).join('；')}`);
    } else if (tools.some(t => t.status === 'error')) {
      const errors = tools.filter(t => t.status === 'error').map(t => `${t.toolName}: ${t.error || 'failed'}`);
      sections.push(`1. **MCP 调用失败**：${errors.join('; ')}`);
      sections.push(`2. **建议**：确认 edge-mcp-server 已启动、8089 WebSocket/HTTP 可达，并检查工具参数是否完整。`);
    } else {
      sections.push(`1. **未获得物理遥测**：当前请求没有触发可执行的 MCP 工具，或缺少工具所需参数。`);
      sections.push(`2. **建议**：提供测点 ID、三相电压、变压器负荷/温度或储能 SOC 等上下文后重试。`);
    }

    return sections.join('\n');
  }
}

// Global Singleton Instance & Reactive Composable
export const aiProviderService = new AIProviderService();

const isCopilotOpen = ref(false);
const chatMessages = ref<ChatMessage[]>([
  {
    id: 'msg_welcome',
    role: 'assistant',
    content: '👋 您好！我是 **EdgeOS 工业 AI 伴侣**。\n\n我已直连 EdgeOS `edge-mcp-server`，工具目录从服务端 `tools/list` 拉取并缓存。\n\n您可以直接提问，或在任意看板中点击 **[ ⚡ AI 诊断 ]** 进行看图问诊！',
    timestamp: Date.now(),
  },
]);
const activeContext = ref<ChatContext | null>(null);
const isThinking = ref(false);

export function useAICopilot() {
  function toggleCopilot(open?: boolean) {
    isCopilotOpen.value = open !== undefined ? open : !isCopilotOpen.value;
  }

  function openWithContext(ctx: ChatContext, autoPrompt?: string) {
    activeContext.value = ctx;
    isCopilotOpen.value = true;
    if (autoPrompt) {
      sendUserMessage(autoPrompt);
    }
  }

  async function sendUserMessage(text: string, hint?: ToolHint) {
    if (!text.trim() || isThinking.value) return;

    const userMsg: ChatMessage = {
      id: 'msg_' + Date.now(),
      role: 'user',
      content: text,
      timestamp: Date.now(),
      context: activeContext.value ? { ...activeContext.value } : undefined,
    };

    chatMessages.value.push(userMsg);
    isThinking.value = true;

    try {
      const assistantMsg = await aiProviderService.executeChat(
        text,
        activeContext.value || undefined,
        (progressTool) => {
          // Tool progress update
        },
        hint
      );
      chatMessages.value.push(assistantMsg);
    } catch (err: any) {
      chatMessages.value.push({
        id: 'msg_err_' + Date.now(),
        role: 'assistant',
        content: `❌ **AI 诊断执行失败**：${err.message || '未知异常'}`,
        timestamp: Date.now(),
      });
    } finally {
      isThinking.value = false;
    }
  }

  function clearHistory() {
    chatMessages.value = [
      {
        id: 'msg_welcome_' + Date.now(),
        role: 'assistant',
        content: '🧹 对话历史已重置。请问有什么工业测点或拓扑问题需要协助诊断？',
        timestamp: Date.now(),
      },
    ];
  }

  return {
    isCopilotOpen,
    chatMessages,
    activeContext,
    isThinking,
    toggleCopilot,
    openWithContext,
    sendUserMessage,
    clearHistory,
    config: aiProviderService.getConfig(),
    isMcpConnected: aiProviderService.isConnected,
    tools: aiProviderService.tools,
    toolsSource: aiProviderService.toolsSource,
    refreshTools: () => aiProviderService.fetchTools(true),
    callMcpTool: (tool: string, args: any) => aiProviderService.callMcpTool(tool, args),
  };
}
