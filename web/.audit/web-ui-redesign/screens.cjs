const { chromium } = require('playwright');

const BASE = process.env.BASE_URL || 'http://localhost:5173';
const OUT = process.env.OUT_DIR || '/tmp/edgec-shots-after';
const ROUTES = process.env.ROUTES
  ? process.env.ROUTES.split(',')
  : [
      '/dashboard',
      '/data',
      '/energy',
      '/metrics',
      '/events',
      '/devices',
      '/devices/topology',
      '/network',
      '/logs',
      '/scheduling',
      '/storage',
      '/pipelines',
      '/system/users',
      '/system/config',
      '/dcu/packets',
      '/dcu/power-quality',
    ];

(async () => {
  const browser = await chromium.launch({
    executablePath: '/root/.cache/ms-playwright/chromium-1243/chrome-linux64/chrome',
    args: ['--no-sandbox'],
  });
  const page = await browser.newPage({ viewport: { width: 1440, height: 900 } });
  const errors = [];
  page.on('console', (message) => {
    if (message.type() === 'error' && !message.text().includes('8089/mcp')) errors.push(message.text());
  });
  page.on('pageerror', (error) => errors.push('PAGEERROR: ' + error.message));

  await page.goto(BASE + '/#/login', { waitUntil: 'networkidle' });
  await page.locator('[data-testid="login-username"] input').fill('engineer');
  await page.locator('[data-testid="login-password"] input').fill('Engineer-Pass-2026');
  await page.locator('[data-testid="login-submit"]').click();
  await page.waitForTimeout(1500);

  for (const theme of ['light', 'dark']) {
    await page.evaluate((value) => localStorage.setItem('edgeos.theme', value), theme);
    await page.reload({ waitUntil: 'networkidle' });
    await page.waitForTimeout(800);
    for (const route of ROUTES) {
      await page.goto(BASE + '/#' + route, { waitUntil: 'networkidle' });
      await page.waitForTimeout(900);
      const name = `${theme}-${route.replace(/[^a-z0-9]+/gi, '_')}.png`;
      await page.screenshot({ path: `${OUT}/${name}` });
    }
  }

  await page.setViewportSize({ width: 390, height: 844 });
  await page.evaluate(() => localStorage.setItem('edgeos.theme', 'light'));
  await page.reload({ waitUntil: 'networkidle' });
  for (const route of ['/dashboard', '/data', '/network']) {
    await page.goto(BASE + '/#' + route, { waitUntil: 'networkidle' });
    await page.waitForTimeout(700);
    await page.screenshot({ path: `${OUT}/mobile-${route.replace(/[^a-z0-9]+/gi, '_')}.png`, fullPage: true });
  }

  console.log('ERRORS:\n' + [...new Set(errors)].slice(0, 25).join('\n'));
  await browser.close();
})();
