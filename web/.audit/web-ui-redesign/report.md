# Web UI consistency run

Branch: `fix/web-ui-consistency`
Worktree: `/tmp/edgec-web-redesign`

## Definition of done

Both products build, and every registry route renders in light and dark at
1440x900 and 390x844 with no horizontal overflow, readable headings, bundled
Chinese/emoji fonts, a persistent theme toggle, visible focus, and keyboard
dismissable dialogs. Existing unit/e2e gates stay green and the design tooling
boundary/contrast gates stay green.

## What changed

- One Tailwind preset (`web/tooling/tailwind.preset.cjs`) maps the legacy
  palette to semantic tokens. iEMS, DCU and SmartBox share it; the duplicated
  palettes and the unused deep-black token sources are gone.
- DCU now has a Tailwind/PostCSS pipeline, so its 27 Core pages render styled.
- Local font subsets: Noto Sans SC (GB2312 + Latin), JetBrains Mono, and
  monochrome Noto Emoji. No external font/CDN request remains.
- Theme preference: system by default, manual light/dark/system cycle in the
  shell, persisted in `localStorage`, applied before first paint.
- Legacy pages use token surfaces, the colourblind-safe chart palette, Chinese
  page titles matching the menu, and a responsive shell (rail below 1280px,
  drawer below 768px, 1600px content cap).
- Accessibility: global visible focus ring, dialog roles/aria-modal/focus
  traps/Escape for the console, AI drawer and two-man modal, chart text
  alternatives, named icon buttons, and non-tabbable static table rows.
- Charts import only the ECharts modules they use (`echarts/core` + Line,
  Grid, Tooltip, DataZoom, Canvas). The vendor chunk fell from 380.9KB to
  182.4KB gzip, inside the revised 250KB vendor budget.
- The product brand colour is checked against both on-colours at build time,
  and the shell derives the readable on-colour at runtime (fixes dark-theme
  brand buttons).
- Remaining visible chrome on the data, event, topology, power-quality and
  console-adjacent pages is Chinese. Domain abbreviations stay as-is.
- `DataCurves` peak and trend values are computed from the live buffers, not
  hardcoded demo numbers.

## Verification

| Gate | Result |
| --- | --- |
| `npm run build --prefix products/iems` | pass |
| `npm run build --prefix products/dcu` | pass (Tailwind CSS emitted) |
| `npm run build --prefix products/smart_box` | pass |
| `CI=1 npm run test:e2e` (fresh dev servers) | 51 passed |
| `node tooling/cli.mjs all` | pass (inputs, boundaries, 21 contrast pairs) |
| `node tooling/cli.mjs size --product iems` | pass (echarts 182.4KB gzip) |
| `node tooling/cli.mjs size --product dcu` | pass |
| `verify.cjs` iEMS (19 routes x 2 themes + mobile + a11y) | 0 failures |
| `verify.cjs` DCU (17 routes x 2 themes + mobile + a11y) | 0 failures |
| `document.fonts.check('16px "Edge Sans"')` | true |
| `document.fonts.check('16px "Edge Emoji"')` | true |

Run e2e with `CI=1` or a freshly restarted Vite server. Reusing a warm dev
server whose optimized-dependency cache predates the modular ECharts change
produces a false chart failure (`xAxis "0" not found`); a fresh server passes.

Screenshots were captured during the run in `/tmp/edgec-shots-after` and
`/tmp/edgec-dcu-after`; they are intentionally not committed.

## Known limits

- Technical labels (SOC, SOH, LTTB, PCS, protocol names) remain English; that is
  domain vocabulary, not missing localization. A future i18n pass should move
  all visible copy into message catalogs.
- The Sankey widget falls back to simulation when `/api/v1/iems/telemetry` does
  not answer; it labels the mode as `仿真：运行`.
- SmartBox keeps its dark health-hub identity but now shares the token palette
  and font stack.
- The GitNexus graph gate could not run against the worktree. The repo index is
  stale and only knows the main checkout, so builds, e2e and route verification
  were the gate for this change.
