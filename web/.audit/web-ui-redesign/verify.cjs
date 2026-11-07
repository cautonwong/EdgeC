const { chromium } = require('playwright');

const BASE = process.env.BASE_URL || 'http://localhost:5173';
const ROUTES = (process.env.ROUTES || [
  '/dashboard',
  '/data',
  '/data/history',
  '/energy',
  '/metrics',
  '/events',
  '/devices',
  '/devices/topology',
  '/network',
  '/network/cellular',
  '/network/firewall',
  '/logs',
  '/scheduling',
  '/storage',
  '/pipelines',
  '/system/users',
  '/system/config',
  '/dcu/packets',
  '/dcu/power-quality',
].join(',')).split(',');

function parseRgb(value) {
  const match = value.match(/rgba?\(([^)]+)\)/);
  if (!match) return null;
  const parts = match[1].split(',').map((part) => Number(part.trim()));
  return { r: parts[0], g: parts[1], b: parts[2], a: parts.length > 3 ? parts[3] : 1 };
}

function luminance({ r, g, b }) {
  const channel = (value) => {
    const normalized = value / 255;
    return normalized <= 0.03928 ? normalized / 12.92 : ((normalized + 0.055) / 1.055) ** 2.4;
  };
  return 0.2126 * channel(r) + 0.7152 * channel(g) + 0.0722 * channel(b);
}

function contrast(a, b) {
  const l1 = luminance(a);
  const l2 = luminance(b);
  return (Math.max(l1, l2) + 0.05) / (Math.min(l1, l2) + 0.05);
}

(async () => {
  const browser = await chromium.launch({
    executablePath: '/root/.cache/ms-playwright/chromium-1243/chrome-linux64/chrome',
    args: ['--no-sandbox'],
  });
  const page = await browser.newPage({ viewport: { width: 1440, height: 900 } });
  const failures = [];
  const notes = [];

  await page.goto(BASE + '/#/login', { waitUntil: 'networkidle' });
  await page.locator('[data-testid="login-username"] input').fill('engineer');
  await page.locator('[data-testid="login-password"] input').fill('Engineer-Pass-2026');
  await page.locator('[data-testid="login-submit"]').click();
  await page.waitForTimeout(1500);

  const fontOk = await page.evaluate(() => document.fonts.check('16px "Edge Sans"'));
  if (!fontOk) failures.push('font: Edge Sans not loaded');

  for (const theme of ['light', 'dark']) {
    await page.evaluate((value) => localStorage.setItem('edgeos.theme', value), theme);
    await page.reload({ waitUntil: 'networkidle' });
    await page.waitForTimeout(500);
    const applied = await page.evaluate(() => document.documentElement.dataset.theme);
    if (applied !== theme) failures.push(`theme: expected ${theme}, got ${applied}`);

    for (const route of ROUTES) {
      await page.goto(BASE + '/#' + route, { waitUntil: 'networkidle' });
      await page.waitForTimeout(500);
      const result = await page.evaluate(() => {
        const overflow = document.documentElement.scrollWidth - window.innerWidth;
        const heading = document.querySelector('main h1, main h2');
        let headingContrast = null;
        if (heading) {
          const style = getComputedStyle(heading);
          const color = style.color;
          let node = heading;
          let background = null;
          while (node) {
            const bg = getComputedStyle(node).backgroundColor;
            const parsed = bg.match(/rgba?\(([^)]+)\)/);
            if (parsed) {
              const parts = parsed[1].split(',').map((part) => Number(part.trim()));
              const alpha = parts.length > 3 ? parts[3] : 1;
              if (alpha > 0.5) {
                background = bg;
                break;
              }
            }
            node = node.parentElement;
          }
          headingContrast = { color, background };
        }
        return { overflow, headingContrast };
      });
      if (result.overflow > 1) failures.push(`overflow ${route} ${theme}: ${result.overflow}px`);
      if (result.headingContrast?.background) {
        const fg = parseRgb(result.headingContrast.color);
        const bg = parseRgb(result.headingContrast.background);
        if (fg && bg) {
          const ratio = contrast(fg, bg);
          if (ratio < 4.5) {
            failures.push(`contrast ${route} ${theme}: ${ratio.toFixed(2)} (${result.headingContrast.color} on ${result.headingContrast.background})`);
          }
        }
      }
    }
  }

  await page.setViewportSize({ width: 390, height: 844 });
  for (const theme of ['light', 'dark']) {
    await page.evaluate((value) => localStorage.setItem('edgeos.theme', value), theme);
    await page.reload({ waitUntil: 'networkidle' });
    for (const route of ROUTES) {
      await page.goto(BASE + '/#' + route, { waitUntil: 'networkidle' });
      await page.waitForTimeout(300);
      const overflow = await page.evaluate(() => document.documentElement.scrollWidth - window.innerWidth);
      if (overflow > 1) failures.push(`mobile overflow ${route} ${theme}: ${overflow}px`);
    }
  }

  await page.setViewportSize({ width: 1440, height: 900 });
  await page.goto(BASE + '/#/dashboard', { waitUntil: 'networkidle' });
  await page.waitForTimeout(500);

  const unnamed = await page.evaluate(() =>
    [...document.querySelectorAll('button')]
      .filter((button) => !(button.getAttribute('aria-label') || button.getAttribute('title') || button.textContent.trim()))
      .map((button) => button.outerHTML.slice(0, 120)),
  );
  if (unnamed.length) failures.push(`unnamed buttons: ${unnamed.length} ${unnamed[0]}`);

  const tableTabindex = await page.evaluate(() => document.querySelectorAll('table tr[tabindex]').length);
  if (tableTabindex) failures.push(`table rows still tabbable: ${tableTabindex}`);

  await page.locator('[data-testid="theme-toggle"]').click();
  const cycled = await page.evaluate(() => document.documentElement.dataset.theme);
  await page.reload({ waitUntil: 'networkidle' });
  const persisted = await page.evaluate(() => document.documentElement.dataset.theme);
  if (cycled !== persisted) failures.push(`theme persistence: ${cycled} -> ${persisted}`);

  await page.locator('[data-testid="toggle-console"]').click();
  await page.waitForTimeout(400);
  const dialog = await page.evaluate(() => {
    const node = document.querySelector('[role="dialog"]');
    return node ? { label: node.getAttribute('aria-label'), modal: node.getAttribute('aria-modal') } : null;
  });
  if (!dialog || dialog.modal !== 'true') failures.push('console dialog missing role/aria-modal');
  await page.keyboard.press('Escape');
  await page.waitForTimeout(300);
  const dialogClosed = await page.evaluate(() => !document.querySelector('[role="dialog"]'));
  if (!dialogClosed) failures.push('console dialog did not close on Escape');

  await page.locator('[data-testid="toggle-copilot"]').click();
  await page.waitForTimeout(400);
  const aiDialog = await page.evaluate(() => !!document.querySelector('[aria-label="AI 副驾"]'));
  if (!aiDialog) failures.push('AI drawer missing dialog semantics');
  await page.keyboard.press('Escape');
  await page.waitForTimeout(900);
  const aiClosed = await page.evaluate(() => !document.querySelector('[aria-label="AI 副驾"]'));
  if (!aiClosed) failures.push('AI drawer did not close on Escape');

  console.log(JSON.stringify({ failures, notes }, null, 2));
  await browser.close();
  process.exit(failures.length ? 1 : 0);
})();
