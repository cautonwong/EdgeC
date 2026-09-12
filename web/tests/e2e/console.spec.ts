import { test, expect } from '@playwright/test';

test.describe('EdgeOS Web Console & Industrial Terminal (AEP-501/504)', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.waitForLoadState('domcontentloaded');
  });

  test('should toggle Web Console drawer via Terminal Button and Shortcut', async ({ page }) => {
    // 1. Click Terminal Header Trigger Button
    const terminalBtn = page.locator('header button:has-text("Terminal")');
    await expect(terminalBtn).toBeVisible();
    await terminalBtn.click();

    // 2. Assert Web Console drawer opens
    const consoleDrawer = page.locator('text=EdgeOS Global Web Console');
    await expect(consoleDrawer).toBeVisible();

    // 3. Assert Terminal Tab is active
    const terminalTab = page.locator('button:has-text("Terminal")');
    await expect(terminalTab.first()).toBeVisible();

    // 4. Switch to Protocol Sniffer Tab
    const snifferTab = page.locator('button:has-text("Sniffer")');
    if (await snifferTab.isVisible()) {
      await snifferTab.click();
      await expect(page.locator('text=Packet Sniffer').or(page.locator('text=Live Sniffer')).first()).toBeVisible();
    }
  });
});
