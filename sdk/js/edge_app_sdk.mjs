import net from "node:net";

export const ABI_VERSION = "1.0.0";

export class SdkError extends Error {
  constructor(code) {
    super(code);
    this.code = code;
  }
}

export function encodeValue(value) {
  if (typeof value === "boolean") return { boolean: value };
  if (typeof value === "number") {
    return Number.isInteger(value) ? { integer: value } : { float: value };
  }
  if (typeof value === "string") return { text: value };
  throw new TypeError(`unsupported point value: ${typeof value}`);
}

export function decodeValue(encoded) {
  for (const kind of ["float", "integer", "boolean", "text"]) {
    if (kind in encoded) return encoded[kind];
  }
  throw new SdkError("protocol");
}

export class AppSdk {
  constructor(socket) {
    this.socket = socket;
    this.nextId = 1;
    this.pending = new Map();
    this.exports = {};
    this.buffer = Buffer.alloc(0);
    socket.on("data", (chunk) => this._onData(chunk));
    socket.on("error", (err) => this._fail(err));
    socket.on("close", () => this._fail(new SdkError("transport")));
  }

  static connect(path) {
    return new Promise((resolve, reject) => {
      const socket = net.connect(path, () => resolve(new AppSdk(socket)));
      socket.once("error", reject);
    });
  }

  close() {
    this.socket.end();
  }

  _fail(err) {
    for (const pending of this.pending.values()) pending.reject(err);
    this.pending.clear();
  }

  _onData(chunk) {
    this.buffer = Buffer.concat([this.buffer, chunk]);
    while (this.buffer.length >= 4) {
      const size = this.buffer.readUInt32BE(0);
      if (this.buffer.length < 4 + size) return;
      const payload = this.buffer.subarray(4, 4 + size);
      this.buffer = this.buffer.subarray(4 + size);

      let response;
      try {
        response = JSON.parse(payload.toString("utf8"));
      } catch {
        this._fail(new SdkError("protocol"));
        return;
      }
      const pending = this.pending.get(response.id);
      if (!pending) continue;
      this.pending.delete(response.id);
      if (response.ok) pending.resolve(response.result ?? {});
      else pending.reject(new SdkError(response.code ?? "internal"));
    }
  }

  _call(fn, args) {
    const id = this.nextId++;
    const body = Buffer.from(JSON.stringify({ id, fn, args }), "utf8");
    const prefix = Buffer.alloc(4);
    prefix.writeUInt32BE(body.length, 0);
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.socket.write(Buffer.concat([prefix, body]));
    });
  }

  setExports({ init, onEvent, stop } = {}) {
    this.exports = { init, onEvent, stop };
  }

  async dispatch(request) {
    const { id = 0, fn, args = {} } = request;
    switch (fn) {
      case "app_init":
        this.exports.init?.();
        break;
      case "app_on_event":
        await this.exports.onEvent?.(args);
        break;
      case "app_stop":
        this.exports.stop?.();
        break;
      default:
        return { id, ok: false, code: "not_found" };
    }
    return { id, ok: true, result: {} };
  }

  async pointResolve(name) {
    const result = await this._call("point_resolve", { name });
    return { handle: result.handle, access: result.access };
  }

  async pointRead(handle) {
    const result = await this._call("point_read", { handle });
    return {
      value: decodeValue(result.value),
      quality: result.quality,
      source_ts: result.source_ts ?? 0,
    };
  }

  async pointSubscribe(handle, deadband = 0, minIntervalMs = 0, maxIntervalMs = 0) {
    const result = await this._call("point_subscribe", {
      handle,
      deadband,
      min_interval_ms: minIntervalMs,
      max_interval_ms: maxIntervalMs,
    });
    return result.subscription;
  }

  async pointPublish(handle, value, quality = "good", sourceTs = 0) {
    await this._call("point_publish", {
      handle,
      sample: { value: encodeValue(value), quality, source_ts: sourceTs },
    });
  }

  async log(level, message) {
    await this._call("log", { level, message });
  }

  async now() {
    return (await this._call("now", {})).now_ms;
  }
}
