/**
 * Playwright E2E Test Suite for AEP-504: Industrial RBAC, Two-Man Rule, and Immutable Audit Trail
 */
import { test, expect } from '@playwright/test';
import fs from 'fs';
import path from 'path';

const EVIDENCE_DIR = path.resolve('/workspaces/edgeos/EdgeC/docs/evidence/AEP-504');
if (!fs.existsSync(EVIDENCE_DIR)) {
  fs.mkdirSync(EVIDENCE_DIR, { recursive: true });
}

test.describe('AEP-504: Industrial Console RBAC & Two-Man Rule Interlock', () => {

  test('Complete RBAC & Two-Man Rule End-to-End Flow', async ({ page }) => {
    // Navigate to local SPA root
    await page.goto('http://localhost:5173/');
    await page.waitForLoadState('networkidle');

    // 1. Open Console Drawer using hotkey Ctrl + `
    await page.keyboard.press('Control+`');
    await page.waitForTimeout(500);

    // Verify Console is visible
    const consoleBrand = page.locator('text=EdgeOS Global Web Console');
    await expect(consoleBrand).toBeVisible();

    // Capture initial console state (Admin)
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '01_admin_console_ready.png') });

    // 2. Test 'auth whoami' command
    const shellInput = page.locator('input[placeholder*="Type command"]');
    await shellInput.fill('auth whoami');
    await shellInput.press('Enter');
    await page.waitForTimeout(400);

    // Verify whoami output
    const authOutput = page.locator('text=Authentication & RBAC Identity');
    await expect(authOutput).toBeVisible();
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '02_auth_whoami_admin.png') });

    // 3. Switch Role to Operator
    const roleSelect = page.locator('select');
    await roleSelect.selectOption('Operator');
    await page.waitForTimeout(300);

    // Run read-only command as Operator (Permitted)
    await shellInput.fill('datahub stats');
    await shellInput.press('Enter');
    await page.waitForTimeout(400);
    await expect(page.locator('text=DataHub Shadow Bus Summary')).toBeVisible();

    // 4. Operator attempts restricted write command (RBAC BLOCKED)
    await shellInput.fill('tag write VOLTAGE_PHASE_A 230.0');
    await shellInput.press('Enter');
    await page.waitForTimeout(400);

    // Verify RBAC blocked message & badge
    const blockedMsg = page.locator('text=RBAC ERROR: 403 FORBIDDEN');
    await expect(blockedMsg).toBeVisible();
    const blockedBadge = page.locator('text=RBAC BLOCKED').first();
    await expect(blockedBadge).toBeVisible();
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '03_operator_rbac_blocked.png') });

    // 5. Elevate Role to Engineer
    await roleSelect.selectOption('Engineer');
    await page.waitForTimeout(300);

    // 6. Engineer runs high-risk command -> Triggers Two-Man Rule Modal
    await shellInput.fill('tag write PCS_TARGET_POWER 350.0');
    await shellInput.press('Enter');
    await page.waitForTimeout(500);

    // Verify Two-Man Rule Modal is displayed
    const twoManHeader = page.locator('text=Two-Man Rule Dual Authorization');
    await expect(twoManHeader).toBeVisible();
    await expect(page.locator('text=tag write PCS_TARGET_POWER 350.0')).toBeVisible();
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '04_two_man_rule_modal_popup.png') });

    // 7. Test Self-Review Validation Rejection
    const supUserInput = page.locator('input[placeholder*="supervisor_admin"]');
    const supTokenInput = page.locator('input[placeholder*="6-digit TOTP"]');
    const authBtn = page.locator('button:has-text("Dual Authorize & Execute")');

    // Fill same name as initiator ('lead_eng')
    await supUserInput.fill('lead_eng');
    await supTokenInput.fill('624430');
    await authBtn.click();
    await page.waitForTimeout(300);

    // Verify self-review error
    const selfReviewErr = page.locator('text=Supervisor cannot be identical to the initiator');
    await expect(selfReviewErr).toBeVisible();
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '05_self_review_rejected.png') });

    // 8. Test Valid Supervisor Approval
    await supUserInput.fill('chief_supervisor_wang');
    await supTokenInput.fill('624430');
    await authBtn.click();
    await page.waitForTimeout(500);

    // Modal should close and command should execute with 2-MAN OK badge
    await expect(twoManHeader).not.toBeVisible();
    const twoManOkBadge = page.locator('text=2-MAN OK').first();
    await expect(twoManOkBadge).toBeVisible();
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '06_two_man_authorized_executed.png') });

    // 9. Inspect Immutable Audit Trail
    await shellInput.fill('audit log');
    await shellInput.press('Enter');
    await page.waitForTimeout(400);

    const auditHeader = page.locator('text=Immutable Console Audit Trail');
    await expect(auditHeader).toBeVisible();
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '07_audit_trail_view.png') });

    // 10. Switch Role to Observer -> Verify Web Console restricted view
    await roleSelect.selectOption('Observer');
    await page.waitForTimeout(400);

    const observerBanner = page.locator('text=Interactive Web Shell Restricted');
    await expect(observerBanner).toBeVisible();
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '08_observer_restricted_view.png') });
  });

});
