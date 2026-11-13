package platformbackup

import (
	"context"
	"database/sql"
	"encoding/json"
	"errors"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

func createTestDB(t *testing.T, path string) {
	t.Helper()
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		t.Fatal(err)
	}
	defer db.Close()

	stmts := []string{
		"PRAGMA journal_mode = WAL",
		"PRAGMA user_version = 5",
		`CREATE TABLE edge_settings (product TEXT, tag_id INTEGER, val BLOB, PRIMARY KEY (product, tag_id))`,
		`CREATE TABLE app_registry (id TEXT PRIMARY KEY, name TEXT, version TEXT, trigger_tag INTEGER)`,
		`CREATE TABLE app_kv (app_id TEXT, key TEXT, val TEXT, persist INTEGER, PRIMARY KEY (app_id, key))`,
		`CREATE TABLE app_secret (app_id TEXT, name TEXT, value TEXT)`,
		`CREATE TABLE app_grant (app_id TEXT, capability TEXT, scope_json TEXT, PRIMARY KEY (app_id, capability))`,
		`CREATE TABLE telemetry (ts INTEGER, tag_id INTEGER, val REAL, PRIMARY KEY (ts, tag_id))`,
		`CREATE TABLE access_stats (app_id TEXT, hits INTEGER)`,
		`CREATE TABLE sessions (id TEXT PRIMARY KEY, token TEXT)`,
		`INSERT INTO edge_settings VALUES ('iems', 1, 'configured')`,
		`INSERT INTO app_registry VALUES ('demo', 'Demo', '1.0.0', 42)`,
		`INSERT INTO app_kv VALUES ('demo', 'keep', 'persisted', 1)`,
		`INSERT INTO app_kv VALUES ('demo', 'cache', 'drop-me', 0)`,
		`INSERT INTO app_secret VALUES ('demo', 'cloud', 'ciphertext-marker')`,
		`INSERT INTO app_grant VALUES ('demo', 'tags.read', '["U_A"]')`,
		`INSERT INTO telemetry VALUES (1, 1001, 1.5)`,
		`INSERT INTO access_stats VALUES ('demo', 99)`,
		`INSERT INTO sessions VALUES ('s1', 'plaintext-token-marker')`,
	}
	for _, stmt := range stmts {
		if _, err := db.Exec(stmt); err != nil {
			t.Fatalf("%s: %v", stmt, err)
		}
	}
}

func openDB(t *testing.T, path string) *sql.DB {
	t.Helper()
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { db.Close() })
	return db
}

func queryInt(t *testing.T, db *sql.DB, query string, args ...any) int {
	t.Helper()
	var n int
	if err := db.QueryRow(query, args...).Scan(&n); err != nil {
		t.Fatalf("%s: %v", query, err)
	}
	return n
}

func queryString(t *testing.T, db *sql.DB, query string, args ...any) string {
	t.Helper()
	var value string
	if err := db.QueryRow(query, args...).Scan(&value); err != nil {
		t.Fatalf("%s: %v", query, err)
	}
	return value
}

func hasTable(t *testing.T, db *sql.DB, name string) bool {
	t.Helper()
	return queryInt(t, db, `SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?`, name) > 0
}

func assertIntegrity(t *testing.T, db *sql.DB) {
	t.Helper()
	var result string
	if err := db.QueryRow("PRAGMA integrity_check").Scan(&result); err != nil {
		t.Fatal(err)
	}
	if result != "ok" {
		t.Fatalf("integrity_check = %q", result)
	}
}

func rewriteManifest(t *testing.T, dir string, mutate func(*Manifest)) {
	t.Helper()
	m, err := ReadManifest(dir)
	if err != nil {
		t.Fatal(err)
	}
	mutate(m)
	data, err := json.MarshalIndent(m, "", "  ")
	if err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(dir, manifestName), data, 0600); err != nil {
		t.Fatal(err)
	}
}

func TestCreateManifestAndSnapshot(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "iems.db")
	createTestDB(t, src)
	dest := filepath.Join(dir, "backup")

	m, err := Create(context.Background(), CreateOptions{
		DBPath: src, DestDir: dest, PlatformVersion: "5.0.0",
	})
	if err != nil {
		t.Fatal(err)
	}
	if m.FormatVersion != FormatVersion {
		t.Errorf("format version = %d, want %d", m.FormatVersion, FormatVersion)
	}
	if m.PlatformVersion != "5.0.0" {
		t.Errorf("platform version = %q", m.PlatformVersion)
	}
	if m.SchemaVersion != 5 {
		t.Errorf("schema version = %d, want 5", m.SchemaVersion)
	}
	if m.CreatedAt.IsZero() {
		t.Error("created_at is zero")
	}
	if len(m.Files) != 1 || m.Files[0].Name != SnapshotName {
		t.Fatalf("files = %+v", m.Files)
	}
	info, err := os.Stat(filepath.Join(dest, SnapshotName))
	if err != nil {
		t.Fatal(err)
	}
	if m.Files[0].Size != info.Size() {
		t.Errorf("manifest size = %d, file size = %d", m.Files[0].Size, info.Size())
	}
	sum, err := sha256File(filepath.Join(dest, SnapshotName))
	if err != nil {
		t.Fatal(err)
	}
	if m.Files[0].SHA256 != sum {
		t.Errorf("manifest sha256 = %s, file sha256 = %s", m.Files[0].SHA256, sum)
	}

	snap := openDB(t, filepath.Join(dest, SnapshotName))
	assertIntegrity(t, snap)
	for _, name := range []string{"edge_settings", "app_registry", "app_kv", "app_secret"} {
		if !hasTable(t, snap, name) {
			t.Errorf("control table %s missing from snapshot", name)
		}
	}
	for _, name := range []string{"telemetry", "access_stats", "sessions"} {
		if hasTable(t, snap, name) {
			t.Errorf("excluded table %s leaked into snapshot", name)
		}
	}
	if got := queryInt(t, snap, `SELECT COUNT(*) FROM app_kv`); got != 1 {
		t.Errorf("app_kv rows = %d, want only persist=1 row", got)
	}
	if got := queryInt(t, snap, `SELECT COUNT(*) FROM app_kv WHERE key='keep'`); got != 1 {
		t.Error("persist=1 row missing")
	}

	raw, err := os.ReadFile(filepath.Join(dest, SnapshotName))
	if err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(string(raw), "ciphertext-marker") {
		t.Error("secret ciphertext missing from backup")
	}
	for _, leaked := range []string{"plaintext-token-marker", "drop-me"} {
		if strings.Contains(string(raw), leaked) {
			t.Errorf("excluded value %q leaked into backup", leaked)
		}
	}
}

func TestBackupWhileWritesInFlight(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "iems.db")
	createTestDB(t, src)

	writer := openDB(t, src)
	if _, err := writer.Exec("PRAGMA busy_timeout = 5000"); err != nil {
		t.Fatal(err)
	}
	stop := make(chan struct{})
	done := make(chan struct{})
	go func() {
		defer close(done)
		for i := 0; ; i++ {
			select {
			case <-stop:
				return
			default:
			}
			if _, err := writer.Exec(
				`INSERT OR REPLACE INTO edge_settings VALUES ('iems', ?, 'during-write')`, 1000+i); err != nil {
				t.Errorf("writer: %v", err)
				return
			}
			time.Sleep(time.Millisecond)
		}
	}()
	time.Sleep(10 * time.Millisecond)

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()
	m, err := Create(ctx, CreateOptions{
		DBPath: src, DestDir: filepath.Join(dir, "backup"), PlatformVersion: "5.0.0",
	})
	close(stop)
	<-done
	if err != nil {
		t.Fatal(err)
	}
	if m.SchemaVersion != 5 {
		t.Errorf("schema version = %d", m.SchemaVersion)
	}

	snap := openDB(t, filepath.Join(dir, "backup", SnapshotName))
	assertIntegrity(t, snap)
	if got := queryInt(t, snap, `SELECT COUNT(*) FROM edge_settings`); got < 1 {
		t.Errorf("edge_settings rows = %d", got)
	}
}

func TestRestoreToEmptyDevice(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "iems.db")
	createTestDB(t, src)
	backupDir := filepath.Join(dir, "backup")
	if _, err := Create(context.Background(), CreateOptions{
		DBPath: src, DestDir: backupDir, PlatformVersion: "5.0.0",
	}); err != nil {
		t.Fatal(err)
	}

	targetDir := filepath.Join(dir, "restored")
	if err := os.MkdirAll(targetDir, 0700); err != nil {
		t.Fatal(err)
	}
	target := filepath.Join(targetDir, "iems.db")
	rotations := 0
	result, err := Restore(context.Background(), RestoreOptions{
		BackupDir:              backupDir,
		DBPath:                 target,
		PlatformVersion:        "5.0.0",
		SupportedSchemaVersion: 5,
		RotateTokens: func() error {
			rotations++
			return nil
		},
	})
	if err != nil {
		t.Fatal(err)
	}
	if !result.SessionTokensRotated || rotations != 1 {
		t.Errorf("token rotation: result=%v calls=%d", result.SessionTokensRotated, rotations)
	}

	restored := openDB(t, target)
	assertIntegrity(t, restored)
	if got := queryInt(t, restored, "PRAGMA user_version"); got != 5 {
		t.Errorf("restored user_version = %d", got)
	}
	if got := queryInt(t, restored, `SELECT COUNT(*) FROM edge_settings WHERE product='iems'`); got != 1 {
		t.Errorf("edge_settings rows = %d", got)
	}
	if got := queryInt(t, restored, `SELECT COUNT(*) FROM app_grant`); got != 1 {
		t.Errorf("app_grant rows = %d", got)
	}
	if got := queryInt(t, restored, `SELECT COUNT(*) FROM app_registry WHERE id='demo'`); got != 1 {
		t.Errorf("app_registry rows = %d", got)
	}
	if got := queryInt(t, restored, `SELECT COUNT(*) FROM app_kv WHERE key='keep'`); got != 1 {
		t.Errorf("persisted KV rows = %d", got)
	}
	for _, name := range []string{"telemetry", "access_stats", "sessions"} {
		if hasTable(t, restored, name) {
			t.Errorf("excluded table %s restored", name)
		}
	}
}

func TestRestoreRejectsNewerPlatform(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "iems.db")
	createTestDB(t, src)
	backupDir := filepath.Join(dir, "backup")
	if _, err := Create(context.Background(), CreateOptions{
		DBPath: src, DestDir: backupDir, PlatformVersion: "6.0.0",
	}); err != nil {
		t.Fatal(err)
	}

	target := filepath.Join(dir, "empty.db")
	called := false
	_, err := Restore(context.Background(), RestoreOptions{
		BackupDir:              backupDir,
		DBPath:                 target,
		PlatformVersion:        "5.0.0",
		SupportedSchemaVersion: 6,
		RotateTokens: func() error {
			called = true
			return nil
		},
	})
	if !errors.Is(err, ErrBackupTooNew) {
		t.Fatalf("err = %v, want ErrBackupTooNew", err)
	}
	if called {
		t.Error("token rotation ran for a rejected backup")
	}
	if _, statErr := os.Stat(target); !os.IsNotExist(statErr) {
		t.Errorf("rejected restore touched the target database: %v", statErr)
	}
}

func TestRestoreRejectsNewerSchema(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "iems.db")
	createTestDB(t, src)
	backupDir := filepath.Join(dir, "backup")
	if _, err := Create(context.Background(), CreateOptions{
		DBPath: src, DestDir: backupDir, PlatformVersion: "5.0.0",
	}); err != nil {
		t.Fatal(err)
	}
	rewriteManifest(t, backupDir, func(m *Manifest) { m.SchemaVersion = 6 })

	target := filepath.Join(dir, "empty.db")
	_, err := Restore(context.Background(), RestoreOptions{
		BackupDir:              backupDir,
		DBPath:                 target,
		PlatformVersion:        "5.0.0",
		SupportedSchemaVersion: 5,
	})
	if !errors.Is(err, ErrBackupTooNew) {
		t.Fatalf("err = %v, want ErrBackupTooNew", err)
	}
	if _, statErr := os.Stat(target); !os.IsNotExist(statErr) {
		t.Errorf("rejected restore touched the target database: %v", statErr)
	}
}

func TestRestoreRejectsTamperedSnapshot(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "iems.db")
	createTestDB(t, src)
	backupDir := filepath.Join(dir, "backup")
	if _, err := Create(context.Background(), CreateOptions{
		DBPath: src, DestDir: backupDir, PlatformVersion: "5.0.0",
	}); err != nil {
		t.Fatal(err)
	}
	snapshotPath := filepath.Join(backupDir, SnapshotName)
	raw, err := os.ReadFile(snapshotPath)
	if err != nil {
		t.Fatal(err)
	}
	raw[len(raw)/2] ^= 0xff
	if err := os.WriteFile(snapshotPath, raw, 0600); err != nil {
		t.Fatal(err)
	}

	target := filepath.Join(dir, "empty.db")
	_, err = Restore(context.Background(), RestoreOptions{
		BackupDir:              backupDir,
		DBPath:                 target,
		PlatformVersion:        "5.0.0",
		SupportedSchemaVersion: 5,
	})
	if !errors.Is(err, ErrHashMismatch) {
		t.Fatalf("err = %v, want ErrHashMismatch", err)
	}
	if _, statErr := os.Stat(target); !os.IsNotExist(statErr) {
		t.Errorf("rejected restore touched the target database: %v", statErr)
	}
}

func TestRestoreCreatesPreRestoreBackup(t *testing.T) {
	dir := t.TempDir()
	src := filepath.Join(dir, "iems.db")
	createTestDB(t, src)
	backupDir := filepath.Join(dir, "backup")
	if _, err := Create(context.Background(), CreateOptions{
		DBPath: src, DestDir: backupDir, PlatformVersion: "5.0.0",
	}); err != nil {
		t.Fatal(err)
	}

	targetDir := filepath.Join(dir, "device")
	if err := os.MkdirAll(targetDir, 0700); err != nil {
		t.Fatal(err)
	}
	target := filepath.Join(targetDir, "iems.db")
	if _, err := Restore(context.Background(), RestoreOptions{
		BackupDir: backupDir, DBPath: target, PlatformVersion: "5.0.0", SupportedSchemaVersion: 5,
	}); err != nil {
		t.Fatal(err)
	}

	live := openDB(t, target)
	if _, err := live.Exec(`UPDATE edge_settings SET val='before-restore' WHERE product='iems'`); err != nil {
		t.Fatal(err)
	}
	live.Close()

	safetyDir := filepath.Join(dir, "safety")
	result, err := Restore(context.Background(), RestoreOptions{
		BackupDir:              backupDir,
		DBPath:                 target,
		PlatformVersion:        "5.0.0",
		SupportedSchemaVersion: 5,
		PreRestoreBackupDir:    safetyDir,
	})
	if err != nil {
		t.Fatal(err)
	}
	if result.SafetyBackupDir != safetyDir {
		t.Errorf("safety backup dir = %q, want %q", result.SafetyBackupDir, safetyDir)
	}
	safety := openDB(t, filepath.Join(safetyDir, SnapshotName))
	if got := queryString(t, safety, `SELECT val FROM edge_settings WHERE product='iems'`); got != "before-restore" {
		t.Errorf("safety backup value = %q, want before-restore", got)
	}
	restored := openDB(t, target)
	if got := queryString(t, restored, `SELECT val FROM edge_settings WHERE product='iems'`); got != "configured" {
		t.Errorf("restored value = %q, want configured", got)
	}
}

func TestRotateTokenFile(t *testing.T) {
	path := filepath.Join(t.TempDir(), "session.token")
	if err := os.WriteFile(path, []byte("old-token\n"), 0600); err != nil {
		t.Fatal(err)
	}
	if err := RotateTokenFile(path); err != nil {
		t.Fatal(err)
	}
	data, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	if strings.TrimSpace(string(data)) == "old-token" {
		t.Error("token was not rotated")
	}
	if len(strings.TrimSpace(string(data))) != 64 {
		t.Errorf("token length = %d, want 64 hex chars", len(strings.TrimSpace(string(data))))
	}
	info, err := os.Stat(path)
	if err != nil {
		t.Fatal(err)
	}
	if info.Mode().Perm() != 0600 {
		t.Errorf("token mode = %o, want 0600", info.Mode().Perm())
	}
}
