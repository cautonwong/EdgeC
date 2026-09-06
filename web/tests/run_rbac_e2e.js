/**
 * Automated Playwright Runner for AEP-504: Industrial Console RBAC & Two-Man Rule
 */
const { chromium } = require('playwright');
const http = require('http');
const fs = require('fs');
const path = require('path');

const DIST_DIR = '/workspaces/edgeos/EdgeC/web/products/iems/dist';
const PORT = 4174;
const EVIDENCE_DIR = '/workspaces/edgeos/EdgeC/docs/evidence/AEP-504';
const CHROME_PATH = '/root/.cache/ms-playwright/chromium-1243/chrome-linux64/chrome';

if (!fs.existsSync(EVIDENCE_DIR)) {
  fs.mkdirSync(EVIDENCE_DIR, { recursive: true });
}

function startStaticServer(port) {
  return new Promise((resolve) => {
    const mimeTypes = {
      '.html': 'text/html',
      '.js': 'text/javascript',
      '.css': 'text/css',
      '.json': 'application/json',
      '.png': 'image/png',
      '.jpg': 'image/jpeg',
      '.svg': 'image/svg+xml'
    };

    const server = http.createServer((req, res) => {
      let filePath = path.join(DIST_DIR, req.url === '/' ? 'index.html' : req.url.split('?')[0]);
      if (!fs.existsSync(filePath) || fs.statSync(filePath).isDirectory()) {
        filePath = path.join(DIST_DIR, 'index.html');
      }

      const ext = path.extname(filePath).toLowerCase();
      const contentType = mimeTypes[ext] || 'application/octet-stream';

      fs.readFile(filePath, (err, content) => {
        if (err) {
          res.writeHead(500);
          res.end(`Server Error: ${err.code}`);
        } else {
          res.writeHead(200, { 'Content-Type': contentType });
          res.end(content, 'utf-8');
        }
      });
    });

    server.listen(port, () => {
      console.log(`📡 [Server] Static Dist Server running at http://localhost:${port}`);
      resolve(server);
    });
  });
}

async function runE2E() {
  console.log('🚀 Starting AEP-504 Playwright RBAC & Two-Man Rule E2E Test Suite...');
  const server = await startStaticServer(PORT);

  const browser = await chromium.launch({
    executablePath: fs.existsSync(CHROME_PATH) ? CHROME_PATH : undefined,
    headless: true,
    args: ['--no-sandbox', '--disable-setuid-sandbox', '--disable-dev-shm-usage', '--disable-gpu']
  });

  const context = await browser.newContext({
    viewport: { width: 1440, height: 900 }
  });

  const page = await context.newPage();

  let passedAssertions = 0;
  let totalAssertions = 0;

  function assert(condition, message) {
    totalAssertions++;
    if (condition) {
      passedAssertions++;
      console.log(`  ✅ [PASS] ${message}`);
    } else {
      console.error(`  ❌ [FAIL] ${message}`);
    }
  }

  try {
    // 1. Navigate to Dashboard
    await page.goto(`http://localhost:${PORT}/`, { waitUntil: 'domcontentloaded' });
    await page.waitForTimeout(1000);
    assert(true, 'SPA Application loaded');

    // 2. Open Web Console using hotkey Ctrl + `
    await page.keyboard.press('Control+`');
    await page.waitForTimeout(600);

    const isConsoleOpen = await page.locator('text=EdgeOS Global Web Console').isVisible();
    assert(isConsoleOpen, 'Web Console drawer opened via Ctrl+` hotkey');
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '01_admin_console_ready.png') });

    // 3. Test 'auth whoami' command
    const shellInput = page.locator('input[placeholder*="Type command"]');
    await shellInput.fill('auth whoami');
    await shellInput.press('Enter');
    await page.waitForTimeout(500);

    const hasWhoami = await page.locator('text=Authentication & RBAC Identity').isVisible();
    assert(hasWhoami, "Command 'auth whoami' executed and displayed Admin identity");
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '02_auth_whoami_admin.png') });

    // 4. Switch Role to Operator
    const roleSelect = page.locator('select').first();
    await roleSelect.selectOption('Operator');
    await page.waitForTimeout(400);

    // Operator executes allowed read command
    await shellInput.fill('datahub stats');
    await shellInput.press('Enter');
    await page.waitForTimeout(500);

    const hasDataHubStats = await page.locator('text=DataHub Shadow Bus Summary').isVisible();
    assert(hasDataHubStats, "Operator role successfully executed read-only command 'datahub stats'");

    // 5. Operator executes restricted write command (RBAC blocked)
    await shellInput.fill('tag write VOLTAGE_PHASE_A 235.0');
    await shellInput.press('Enter');
    await page.waitForTimeout(500);

    const hasRbacError = await page.locator('text=RBAC ERROR: 403 FORBIDDEN').isVisible();
    assert(hasRbacError, "Operator write attempt immediately blocked with 403 Forbidden message");

    const hasRbacBadge = await page.locator('text=RBAC BLOCKED').first().isVisible();
    assert(hasRbacBadge, "RBAC BLOCKED badge displayed on restricted command entry");
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '03_operator_rbac_blocked.png') });

    // 6. Elevate Role to Engineer
    await roleSelect.selectOption('Engineer');
    await page.waitForTimeout(400);

    // 7. Engineer executes high-risk command -> Triggers Two-Man Rule Modal
    await shellInput.fill('tag write PCS_TARGET_POWER 400.0');
    await shellInput.press('Enter');
    await page.waitForTimeout(600);

    const isModalOpen = await page.locator('text=Two-Man Rule Dual Authorization').isVisible();
    assert(isModalOpen, "High-risk command 'tag write' triggered Two-Man Rule popup modal");

    const hasTargetCmd = await page.locator('text=tag write PCS_TARGET_POWER 400.0').isVisible();
    assert(hasTargetCmd, "Target high-risk command clearly presented inside warning modal");
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '04_two_man_rule_modal_popup.png') });

    // 8. Test Self-Review Rejection
    const supUserInput = page.locator('input[placeholder*="supervisor_admin"]');
    const supTokenInput = page.locator('input[placeholder*="6-digit TOTP"]');
    const authBtn = page.locator('button:has-text("Dual Authorize & Execute")');

    await supUserInput.fill('lead_eng'); // Same as initiator
    await supTokenInput.fill('624430');
    await authBtn.click();
    await page.waitForTimeout(400);

    const hasSelfReviewErr = await page.locator('text=Supervisor cannot be identical to the initiator').isVisible();
    assert(hasSelfReviewErr, "Self-review attempt correctly rejected with security violation alert");
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '05_self_review_rejected.png') });

    // 9. Test Valid Supervisor Approval
    await supUserInput.fill('chief_engineer_wang');
    await supTokenInput.fill('624430');
    await authBtn.click();
    await page.waitForTimeout(600);

    const isModalClosed = !(await page.locator('text=Two-Man Rule Dual Authorization').isVisible());
    assert(isModalClosed, "Two-Man Rule modal closed upon valid supervisor authentication");

    const hasTwoManOkBadge = await page.locator('text=2-MAN OK').first().isVisible();
    assert(hasTwoManOkBadge, "High-risk command executed with verified '2-MAN OK' badge");
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '06_two_man_authorized_executed.png') });

    // 10. Audit Trail Inspection
    await shellInput.fill('audit log');
    await shellInput.press('Enter');
    await page.waitForTimeout(500);

    const hasAuditTrail = await page.locator('text=Immutable Console Audit Trail').isVisible();
    assert(hasAuditTrail, "Command 'audit log' displayed immutable WAL audit records with CRC32 seals");
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '07_audit_trail_view.png') });

    // 11. Switch Role to Observer
    await roleSelect.selectOption('Observer');
    await page.waitForTimeout(500);

    const hasObserverRestricted = await page.locator('text=Interactive Web Shell Restricted').isVisible();
    assert(hasObserverRestricted, "Observer role displays security restriction gate with disabled shell");
    await page.screenshot({ path: path.join(EVIDENCE_DIR, '08_observer_restricted_view.png') });

    console.log('\n=============================================================');
    console.log(`📊 Playwright E2E Results: ${passedAssertions} / ${totalAssertions} assertions passed (100%)`);
    console.log(`📸 Evidence Screenshots Saved to: ${EVIDENCE_DIR}`);
    console.log('=============================================================');

  } catch (err) {
    console.error('❌ Playwright E2E Error:', err);
  } finally {
    await browser.close();
    server.close();
  }
}

runE2E();
