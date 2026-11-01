import { test, expect } from '@playwright/test';

test.describe('EdgeOS Industrial AI Copilot (AEP-507)', () => {
  test.beforeEach(async ({ page }) => {
    // Navigate to IEMS Dashboard
    await page.goto('/');
    await page.waitForLoadState('domcontentloaded');
  });

  test('should toggle AI Copilot drawer via Header Button and Hotkey', async ({ page }) => {
    // 1. Click AI Copilot Header Trigger Button
    const copilotBtn = page.locator('header button:has-text("AI Copilot")');
    await expect(copilotBtn).toBeVisible();
    await copilotBtn.click();

    // 2. Assert Drawer opens
    const drawer = page.locator('span:has-text("EdgeOS AI 工业伴侣")');
    await expect(drawer).toBeVisible();

    // 3. Close with Escape key
    await page.keyboard.press('Escape');
    await expect(drawer).not.toBeVisible();

    // 4. Reopen with Ctrl+K shortcut
    await page.keyboard.press('Control+KeyK');
    await expect(drawer).toBeVisible();

    // 5. Close with Close button
    const closeBtn = page.locator('button[title="Close Drawer (Esc)"]');
    await closeBtn.click();
    await expect(drawer).not.toBeVisible();
  });

  test('should support "看图问诊" Context-Aware Diagnosis from LiveTagCard', async ({ page }) => {
    // 1. Locate first LiveTagCard with AI Diagnostic button
    const aiDiagBtn = page.locator('button:has-text("AI 诊断")').first();
    if (await aiDiagBtn.isVisible()) {
      await aiDiagBtn.click();

      // 2. Assert AI Copilot Drawer automatically slides out
      const drawer = page.locator('span:has-text("EdgeOS AI 工业伴侣")');
      await expect(drawer).toBeVisible();

      // 3. Assert Context Pill is attached with Target Tag Info
      const contextPill = page.locator('text=Context:');
      await expect(contextPill).toBeVisible();

      // 4. Assert conversation receives initial automated diagnostic prompt
      const userMessage = page.locator('.prose, .text-xs:has-text("工况诊断")');
      await expect(userMessage.first()).toBeVisible();
    }
  });

  test('should execute Quick Prompt macro and render MCP Tool Call Cards', async ({ page }) => {
    // 1. Open AI Copilot Drawer
    const copilotBtn = page.locator('header button:has-text("AI Copilot")');
    await copilotBtn.click();

    // 2. Click Quick Macro: "⚡ 全站异常扫描"
    const quickScanBtn = page.locator('button:has-text("全站异常扫描")');
    await expect(quickScanBtn).toBeVisible();
    await quickScanBtn.click();

    // 3. Assert Assistant response renders with Diagnosis report
    await expect(page.locator('text=EdgeOS AI 工业伴侣诊断报告')).toBeVisible({ timeout: 10000 });

    // 4. Assert MCP Tool Call Card is present (e.g. datahub_scan_abnormal or io_list_channels)
    const mcpBadge = page.locator('text=DataHub').or(page.locator('text=IO Bus')).or(page.locator('text=datahub_scan_abnormal')).first();
    await expect(mcpBadge).toBeVisible();
  });

  test('should toggle Provider and Model Settings Panel', async ({ page }) => {
    // 1. Open AI Copilot Drawer
    const copilotBtn = page.locator('header button:has-text("AI Copilot")');
    await copilotBtn.click();

    // 2. Click Settings Button
    const settingsBtn = page.locator('button[title="Provider & Model Settings"]');
    await settingsBtn.click();

    // 3. Assert Settings Panel visible with Provider dropdown
    await expect(page.locator('text=LLM Backend & MCP Settings')).toBeVisible();
    const providerSelect = page.locator('select');
    await expect(providerSelect).toBeVisible();

    // 4. Select DeepSeek Provider
    await providerSelect.selectOption('deepseek');
    await expect(providerSelect).toHaveValue('deepseek');
  });
});
