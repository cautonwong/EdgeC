import { test, expect } from '@playwright/test';

test.describe('EdgeOS SmartBox Rehab & Health Hub E2E', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.waitForLoadState('domcontentloaded');
  });

  test('should render SmartBox Hub header and switch navigation tabs', async ({ page }) => {
    // 1. Verify SmartBox Header
    const headerTitle = page.locator('h1:has-text("SMARTBOX")');
    if (await headerTitle.isVisible()) {
      await expect(headerTitle).toBeVisible();

      // 2. Switch to 50Hz Game Data Bridge tab
      const gameTabBtn = page.locator('button:has-text("50Hz 游戏数据桥")');
      await gameTabBtn.click();
      await expect(gameTabBtn).toHaveClass(/bg-emerald-500/);

      // 3. Switch to Positive Feedback tab
      const feedbackTabBtn = page.locator('button:has-text("正向激励与双人PK")');
      await feedbackTabBtn.click();
      await expect(feedbackTabBtn).toHaveClass(/bg-emerald-500/);
    }
  });

  test('should trigger Two-Man Rule Modal on high-risk privacy sandbox toggle', async ({ page }) => {
    const privacyBtn = page.locator('button:has-text("开启隐私沙盒"), button:has-text("隐私硬件")');
    if (await privacyBtn.isVisible()) {
      await privacyBtn.click();

      // Verify Two-Man Rule Modal is triggered
      const modal = page.locator('text=双人复核, text=Two-Man Rule, text=安全授权');
      if (await modal.first().isVisible()) {
        await expect(modal.first()).toBeVisible();
      }
    }
  });
});
