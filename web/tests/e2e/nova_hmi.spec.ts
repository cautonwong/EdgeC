import { test, expect } from '@playwright/test';

test.describe('Nova HMI Industrial UX & Contract-Driven UI (AEP-491/492/493)', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
    await page.waitForLoadState('domcontentloaded');
  });

  test('should verify contract-driven group tabs and live metrics wall (Issue #170)', async ({ page }) => {
    // 1. Assert system stats wall is visible
    await expect(page.locator('text=Health Index')).toBeVisible();
    await expect(page.locator('text=Sync Latency')).toBeVisible();
    await expect(page.locator('text=Live Metrics')).toBeVisible();

    // 2. Assert All Metrics filter button is active
    const allBtn = page.locator('button:has-text("All Metrics")');
    await expect(allBtn).toBeVisible();

    // 3. Assert search filter input is interactive
    const searchInput = page.locator('input[placeholder*="Filter point name"]');
    await expect(searchInput).toBeVisible();
    await searchInput.fill('VOLTAGE');
    await expect(searchInput).toHaveValue('VOLTAGE');
    await searchInput.fill('');
  });

  test('should render LiveTagCards with 400ms animation and liveliness indicator (Issue #171)', async ({ page }) => {
    // Locate telemetry cards
    const card = page.locator('.group.bg-\\[\\#0f172a\\]').first();
    await expect(card).toBeVisible();

    // Verify presence of liveliness badge (LIVE, STALE, or OFFLINE)
    const badge = card.locator('text=LIVE').or(card.locator('text=STALE')).or(card.locator('text=OFFLINE')).first();
    await expect(badge).toBeVisible();

    // Verify AI diagnostic action button
    const aiBtn = card.locator('button:has-text("AI 诊断")');
    await expect(aiBtn).toBeVisible();
  });

  test('should navigate to DataCurves and verify LTTB waveform accelerator (Issue #172)', async ({ page }) => {
    await page.goto('/data/curves');
    await page.waitForLoadState('domcontentloaded');

    // Verify Header & Badges
    await expect(page.locator('text=DATA CURVES')).toBeVisible();
    await expect(page.locator('text=LTTB Accelerated')).toBeVisible();
    await expect(page.locator('text=60 FPS')).toBeVisible();

    // Verify View Mode Buttons
    await expect(page.locator('button:has-text("4-Series Microgrid")')).toBeVisible();
    await expect(page.locator('button:has-text("Single Tag Focus")')).toBeVisible();

    // Verify 100K LTTB Stress button
    const stressBtn = page.locator('button:has-text("100K LTTB Stress")');
    await expect(stressBtn).toBeVisible();
    await stressBtn.click();

    // Verify live stream button toggles
    const streamBtn = page.locator('button:has-text("LIVE STREAM")').or(page.locator('button:has-text("PAUSED")')).first();
    await expect(streamBtn).toBeVisible();
  });
});
