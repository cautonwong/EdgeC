#!/usr/bin/env node
import fs from "node:fs";
import net from "node:net";
import os from "node:os";
import path from "node:path";

import { AppSdk, SdkError } from "./edge_app_sdk.mjs";

function loadVectors() {
  for (const prefix of ["", "../", "../../"]) {
    const file = path.join(prefix, "sdk/conformance/vectors.json");
    if (fs.existsSync(file)) return JSON.parse(fs.readFileSync(file, "utf8"));
  }
  throw new Error("cannot find sdk/conformance/vectors.json");
}

function deepEqual(a, b) {
  if (a === b) return true;
  if (typeof a !== typeof b || a === null || b === null) return false;
  if (Array.isArray(a) || Array.isArray(b)) {
    if (!Array.isArray(a) || !Array.isArray(b) || a.length !== b.length) return false;
    return a.every((value, index) => deepEqual(value, b[index]));
  }
  if (typeof a === "object") {
    const keys = Object.keys(a);
    if (keys.length !== Object.keys(b).length) return false;
    return keys.every((key) => Object.hasOwn(b, key) && deepEqual(a[key], b[key]));
  }
  return false;
}

function decodeExpected(tagged) {
  for (const kind of ["float", "integer", "boolean", "text"]) {
    if (kind in tagged) return tagged[kind];
  }
  throw new Error(`unknown value tag ${JSON.stringify(tagged)}`);
}

function sendResponse(socket, response) {
  const body = Buffer.from(JSON.stringify(response), "utf8");
  const prefix = Buffer.alloc(4);
  prefix.writeUInt32BE(body.length, 0);
  socket.write(Buffer.concat([prefix, body]));
}

function handleRequest(socket, steps, logs, state, request) {
  const step = steps[state.index];
  if (step === undefined) {
    state.error = new Error(`unexpected extra request ${request.fn}`);
    sendResponse(socket, { id: request.id, ok: false, code: "internal" });
    return;
  }
  if (request.fn !== step.fn) {
    state.error = new Error(
      `step ${state.index} fn got ${request.fn} want ${step.fn}`,
    );
  } else if (!deepEqual(request.args, step.args)) {
    state.error = new Error(
      `step ${state.index} ${step.fn} args got ${JSON.stringify(request.args)} want ${JSON.stringify(step.args)}`,
    );
  }
  if (step.fn === "log") logs.push(step.args.message);

  const response = { id: request.id };
  if (step.expect.ok) {
    response.ok = true;
    response.result = step.expect.result;
  } else {
    response.ok = false;
    response.code = step.expect.code;
  }
  const body = Buffer.from(JSON.stringify(response), "utf8");
  const prefix = Buffer.alloc(4);
  prefix.writeUInt32BE(body.length, 0);
  socket.write(Buffer.concat([prefix, body]));
  state.index++;
}

function listen(socketPath, steps, logs, state) {
  return new Promise((resolve, reject) => {
    const server = net.createServer((socket) => {
      let buffer = Buffer.alloc(0);
      socket.on("data", (chunk) => {
        buffer = Buffer.concat([buffer, chunk]);
        while (buffer.length >= 4) {
          const size = buffer.readUInt32BE(0);
          if (buffer.length < 4 + size) return;
          const payload = buffer.subarray(4, 4 + size);
          buffer = buffer.subarray(4 + size);
          let request;
          try {
            request = JSON.parse(payload.toString("utf8"));
          } catch {
            state.error = new Error("request is not JSON");
            continue;
          }
          handleRequest(socket, steps, logs, state, request);
        }
      });
      socket.on("error", (err) => {
        state.error ??= err;
      });
    });
    server.once("error", reject);
    server.listen(socketPath, () => resolve(server));
  });
}

function callStep(sdk, fn, args) {
  switch (fn) {
    case "point_resolve":
      return sdk.pointResolve(args.name);
    case "point_read":
      return sdk.pointRead(args.handle);
    case "point_subscribe":
      return sdk.pointSubscribe(
        args.handle,
        args.deadband,
        args.min_interval_ms,
        args.max_interval_ms,
      );
    case "point_publish":
      return sdk.pointPublish(
        args.handle,
        decodeExpected(args.sample.value),
        args.sample.quality,
        args.sample.source_ts,
      );
    case "log":
      return sdk.log(args.level, args.message);
    case "now":
      return sdk.now();
    default:
      throw new Error(`unknown fn ${fn}`);
  }
}

async function runStep(sdk, step) {
  const { fn, args, expect } = step;
  if (!expect.ok) {
    try {
      await callStep(sdk, fn, args);
    } catch (err) {
      if (!(err instanceof SdkError) || err.code !== expect.code) {
        throw new Error(
          `${fn}: got ${err.code ?? err.message}, want ${expect.code}`,
        );
      }
      return;
    }
    throw new Error(`${fn}: expected ${expect.code}, call succeeded`);
  }

  const result = await callStep(sdk, fn, args);
  const want = expect.result;
  switch (fn) {
    case "point_resolve":
      if (result.handle !== want.handle || result.access !== want.access) {
        throw new Error(`${fn}: got ${JSON.stringify(result)}`);
      }
      break;
    case "point_read":
      if (
        !deepEqual(result.value, decodeExpected(want.value)) ||
        result.quality !== want.quality ||
        result.source_ts !== want.source_ts
      ) {
        throw new Error(`${fn}: got ${JSON.stringify(result)}`);
      }
      break;
    case "point_subscribe":
      if (result !== want.subscription) throw new Error(`${fn}: got ${result}`);
      break;
    case "point_publish":
    case "log":
      if (result !== undefined) throw new Error(`${fn}: got ${result}`);
      break;
    case "now":
      if (result !== want.now_ms) throw new Error(`${fn}: got ${result}`);
      break;
    default:
      throw new Error(`unknown fn ${fn}`);
  }
}

async function runVector(vector, socketPath) {
  const logs = [];
  const state = { index: 0, error: null };
  const server = await listen(socketPath, vector.steps, logs, state);
  const sdk = await AppSdk.connect(socketPath);
  let appError = null;
  sdk.setExports({
    init() {},
    async onEvent() {
      try {
        for (const step of vector.steps) await runStep(sdk, step);
      } catch (err) {
        appError = err;
      }
    },
    stop() {},
  });
  await sdk.dispatch({ id: 1, fn: "app_init", args: {} });
  await sdk.dispatch({ id: 2, fn: "app_on_event", args: vector.event });
  await sdk.dispatch({ id: 3, fn: "app_stop", args: {} });
  sdk.close();
  await new Promise((resolve) => server.close(resolve));

  if (state.error) throw state.error;
  if (state.index !== vector.steps.length) {
    throw new Error(`host saw ${state.index}/${vector.steps.length} steps`);
  }
  if (appError) throw appError;
  if (!deepEqual(logs, vector.expect_log)) {
    throw new Error(
      `logs got ${JSON.stringify(logs)} want ${JSON.stringify(vector.expect_log)}`,
    );
  }
}

async function main() {
  const vectors = loadVectors().vectors;
  let passed = 0;
  for (const [index, vector] of vectors.entries()) {
    const socketPath = path.join(
      os.tmpdir(),
      `edge-sdk-${process.pid}-${index}.sock`,
    );
    try {
      await runVector(vector, socketPath);
      passed++;
      console.log(`PASS ${vector.name}`);
    } catch (err) {
      console.log(`FAIL ${vector.name}: ${err.message}`);
    } finally {
      try {
        fs.unlinkSync(socketPath);
      } catch {
        // already removed by close
      }
    }
  }
  console.log(`sdk conformance (js): ${passed}/${vectors.length} vectors passed`);
  process.exitCode = passed === vectors.length ? 0 : 1;
}

await main();
