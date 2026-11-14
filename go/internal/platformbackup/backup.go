// Package platformbackup creates and restores platform control-plane backups.
//
// A backup is a directory with an online SQLite snapshot (control.db) and a
// manifest.json that records the platform version, schema version, creation
// time, file list and per-file sha256. Only control tables are kept in the
// snapshot: telemetry history, access statistics, session/token tables and
// application code never enter the backup.
package platformbackup

import (
	"context"
	"crypto/sha256"
	"database/sql"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	sqlite3 "github.com/mattn/go-sqlite3"
)

const (
	// FormatVersion is the backup layout this build can write and read.
	FormatVersion = 1
	// SnapshotName is the SQLite snapshot inside every backup directory.
	SnapshotName = "control.db"
	manifestName = "manifest.json"
	// DefaultPlatformVersion mirrors EDGEOS_SDK_VERSION_STRING in
	// include/edgeos/version.h. EDGE_PLATFORM_VERSION overrides it at runtime.
	DefaultPlatformVersion = "5.0.0"
	// DefaultSchemaVersion mirrors EDGE_DB_SCHEMA_VERSION in
	// modules/edge_infra/storage/sqlite_service.c. EDGE_SCHEMA_VERSION
	// overrides it at runtime; it gates restores onto an empty device where
	// no live database exists to read user_version from.
	DefaultSchemaVersion = 11
)

// Exact allowlist. A new control table must be added here deliberately so
// telemetry, access statistics and token tables never leak into a backup.
// shortcut: the allowlist needs a code change when a control table is added,
// upgrade it to per-table metadata when the schema grows past these tables.
var controlTables = map[string]bool{
	"edge_settings":    true,
	"inventory":        true,
	"app_registry":     true,
	"schema_migration": true,
	"matrix_meta":      true,
	"app_settings":     true,
	"app_kv":           true,
	"app_secret":       true,
	"app_data_meta":    true,
	"app_grant":        true,
	"publisher_trust":  true,
	"point_profile":    true,
}

var (
	ErrInvalidManifest   = errors.New("invalid backup manifest")
	ErrUnsupportedFormat = errors.New("unsupported backup format")
	ErrBackupTooNew      = errors.New("backup is newer than the platform")
	ErrHashMismatch      = errors.New("backup file checksum mismatch")
	ErrTokenRotation     = errors.New("session token rotation failed")
)

// FileEntry is one file listed in the manifest.
type FileEntry struct {
	Name   string `json:"name"`
	Size   int64  `json:"size"`
	SHA256 string `json:"sha256"`
}

// Manifest describes the contents of a backup directory.
type Manifest struct {
	FormatVersion   int         `json:"format_version"`
	PlatformVersion string      `json:"platform_version"`
	SchemaVersion   int         `json:"schema_version"`
	CreatedAt       time.Time   `json:"created_at"`
	Files           []FileEntry `json:"files"`
}

// CreateOptions configures Create.
type CreateOptions struct {
	DBPath          string
	DestDir         string
	PlatformVersion string
}

// RestoreOptions configures Restore.
type RestoreOptions struct {
	BackupDir       string
	DBPath          string
	PlatformVersion string
	// SupportedSchemaVersion gates a backup whose schema is newer than the
	// platform can read. Zero means: use the live database's user_version,
	// and skip the gate when there is no live database yet.
	SupportedSchemaVersion int
	// PreRestoreBackupDir, when set, receives a backup of the current database
	// before it is overwritten. A failed safety backup aborts the restore.
	PreRestoreBackupDir string
	// RotateTokens invalidates existing sessions after the database is
	// restored. Nil skips rotation.
	RotateTokens func() error
}

// RestoreResult reports what a successful restore did.
type RestoreResult struct {
	Manifest             *Manifest `json:"manifest"`
	SessionTokensRotated bool      `json:"session_tokens_rotated"`
	SafetyBackupDir      string    `json:"safety_backup_dir,omitempty"`
}

// ReadManifest reads and validates manifest.json from a backup directory.
func ReadManifest(backupDir string) (*Manifest, error) {
	data, err := os.ReadFile(filepath.Join(backupDir, manifestName))
	if err != nil {
		return nil, fmt.Errorf("%w: %v", ErrInvalidManifest, err)
	}
	var m Manifest
	if err := json.Unmarshal(data, &m); err != nil {
		return nil, fmt.Errorf("%w: %v", ErrInvalidManifest, err)
	}
	if m.FormatVersion <= 0 || m.PlatformVersion == "" || m.SchemaVersion < 0 || len(m.Files) == 0 {
		return nil, fmt.Errorf("%w: missing required fields", ErrInvalidManifest)
	}
	for _, f := range m.Files {
		if f.Name == "" || f.Name == "." || f.Name == ".." ||
			strings.ContainsAny(f.Name, `/\`) || filepath.Base(f.Name) != f.Name {
			return nil, fmt.Errorf("%w: unsafe file name %q", ErrInvalidManifest, f.Name)
		}
		if f.Size < 0 || f.SHA256 == "" {
			return nil, fmt.Errorf("%w: incomplete entry %q", ErrInvalidManifest, f.Name)
		}
	}
	return &m, nil
}

// Create takes a consistent snapshot of dbPath into destDir and writes the
// manifest after pruning non-backup tables and hashing the result.
func Create(ctx context.Context, opts CreateOptions) (*Manifest, error) {
	if opts.DBPath == "" || opts.DestDir == "" || opts.PlatformVersion == "" {
		return nil, errors.New("platformbackup: db path, destination and platform version are required")
	}
	if _, err := os.Stat(opts.DBPath); err != nil {
		return nil, fmt.Errorf("source database: %w", err)
	}
	if err := os.MkdirAll(opts.DestDir, 0700); err != nil {
		return nil, fmt.Errorf("create backup directory: %w", err)
	}

	snapshotPath := filepath.Join(opts.DestDir, SnapshotName)
	for _, suffix := range []string{"", "-wal", "-shm"} {
		_ = os.Remove(snapshotPath + suffix)
	}
	if err := copyDatabase(ctx, opts.DBPath, snapshotPath); err != nil {
		return nil, err
	}
	schemaVersion, err := pruneSnapshot(ctx, snapshotPath)
	if err != nil {
		return nil, err
	}
	// VACUUM and connection close leave no WAL; drop sidecars so every byte of
	// the backup is covered by the manifest hash.
	for _, suffix := range []string{"-wal", "-shm"} {
		_ = os.Remove(snapshotPath + suffix)
	}
	if err := os.Chmod(snapshotPath, 0600); err != nil {
		return nil, fmt.Errorf("protect snapshot: %w", err)
	}
	entry, err := fileEntry(opts.DestDir, SnapshotName)
	if err != nil {
		return nil, err
	}

	manifest := &Manifest{
		FormatVersion:   FormatVersion,
		PlatformVersion: opts.PlatformVersion,
		SchemaVersion:   schemaVersion,
		CreatedAt:       time.Now().UTC(),
		Files:           []FileEntry{entry},
	}
	data, err := json.MarshalIndent(manifest, "", "  ")
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(filepath.Join(opts.DestDir, manifestName), data, 0600); err != nil {
		return nil, fmt.Errorf("write manifest: %w", err)
	}
	return manifest, nil
}

// Restore validates a backup, copies it over dbPath with the SQLite backup API,
// then rotates session tokens. Validation happens before the live database is
// touched, so a rejected backup leaves the device unchanged.
func Restore(ctx context.Context, opts RestoreOptions) (*RestoreResult, error) {
	if opts.BackupDir == "" || opts.DBPath == "" || opts.PlatformVersion == "" {
		return nil, errors.New("platformbackup: backup dir, db path and platform version are required")
	}
	manifest, err := ReadManifest(opts.BackupDir)
	if err != nil {
		return nil, err
	}
	if manifest.FormatVersion > FormatVersion {
		return nil, fmt.Errorf("%w: backup format %d, supported %d",
			ErrUnsupportedFormat, manifest.FormatVersion, FormatVersion)
	}
	if compareVersions(manifest.PlatformVersion, opts.PlatformVersion) > 0 {
		return nil, fmt.Errorf("%w: backup platform %s, current %s",
			ErrBackupTooNew, manifest.PlatformVersion, opts.PlatformVersion)
	}

	limit := opts.SupportedSchemaVersion
	if limit == 0 {
		limit, err = liveSchemaVersion(ctx, opts.DBPath)
		if err != nil {
			return nil, err
		}
	}
	if limit > 0 && manifest.SchemaVersion > limit {
		return nil, fmt.Errorf("%w: backup schema %d, current %d",
			ErrBackupTooNew, manifest.SchemaVersion, limit)
	}

	var snapshot *FileEntry
	for i := range manifest.Files {
		if err := verifyEntry(opts.BackupDir, manifest.Files[i]); err != nil {
			return nil, err
		}
		if manifest.Files[i].Name == SnapshotName {
			snapshot = &manifest.Files[i]
		}
	}
	if snapshot == nil {
		return nil, fmt.Errorf("%w: missing %s", ErrInvalidManifest, SnapshotName)
	}

	result := &RestoreResult{Manifest: manifest}
	if opts.PreRestoreBackupDir != "" {
		if _, err := os.Stat(opts.DBPath); err == nil {
			if _, err := Create(ctx, CreateOptions{
				DBPath:          opts.DBPath,
				DestDir:         opts.PreRestoreBackupDir,
				PlatformVersion: opts.PlatformVersion,
			}); err != nil {
				return nil, fmt.Errorf("pre-restore safety backup: %w", err)
			}
			result.SafetyBackupDir = opts.PreRestoreBackupDir
		} else if !os.IsNotExist(err) {
			return nil, fmt.Errorf("pre-restore safety backup: %w", err)
		}
	}

	// The backup API rolls back its destination transaction on failure, so a
	// failed copy leaves the live database as it was.
	if err := copyDatabase(ctx, filepath.Join(opts.BackupDir, snapshot.Name), opts.DBPath); err != nil {
		return nil, fmt.Errorf("restore database: %w", err)
	}

	if opts.RotateTokens != nil {
		if err := opts.RotateTokens(); err != nil {
			return nil, fmt.Errorf("%w: %v", ErrTokenRotation, err)
		}
		result.SessionTokensRotated = true
	}
	return result, nil
}

// copyDatabase copies srcPath into dstPath with sqlite3_backup.
func copyDatabase(ctx context.Context, srcPath, dstPath string) error {
	srcDB, err := openSQLite(ctx, srcPath)
	if err != nil {
		return fmt.Errorf("open source database: %w", err)
	}
	defer srcDB.Close()

	srcConn, srcSQLite, err := rawSQLiteConn(ctx, srcDB)
	if err != nil {
		return fmt.Errorf("source database: %w", err)
	}
	defer srcConn.Close()

	dstDB, err := openSQLite(ctx, dstPath)
	if err != nil {
		return fmt.Errorf("open destination database: %w", err)
	}
	defer dstDB.Close()

	dstConn, dstSQLite, err := rawSQLiteConn(ctx, dstDB)
	if err != nil {
		return fmt.Errorf("destination database: %w", err)
	}
	defer dstConn.Close()

	backup, err := dstSQLite.Backup("main", srcSQLite, "main")
	if err != nil {
		return fmt.Errorf("start sqlite backup: %w", err)
	}
	finished := false
	finish := func() error {
		finished = true
		return backup.Finish()
	}
	defer func() {
		if !finished {
			_ = backup.Finish()
		}
	}()

	for {
		done, stepErr := backup.Step(-1)
		if stepErr != nil {
			_ = finish()
			return fmt.Errorf("sqlite backup: %w", stepErr)
		}
		if done {
			break
		}
		// Step returns SQLITE_BUSY/LOCKED as (false, nil); retry with context.
		select {
		case <-ctx.Done():
			_ = finish()
			return fmt.Errorf("sqlite backup: %w", ctx.Err())
		case <-time.After(10 * time.Millisecond):
		}
	}
	if err := finish(); err != nil {
		return fmt.Errorf("finish sqlite backup: %w", err)
	}
	return nil
}

func openSQLite(ctx context.Context, path string) (*sql.DB, error) {
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		return nil, err
	}
	db.SetMaxOpenConns(1)
	if err := db.PingContext(ctx); err != nil {
		db.Close()
		return nil, err
	}
	if _, err := db.ExecContext(ctx, "PRAGMA busy_timeout = 5000"); err != nil {
		db.Close()
		return nil, err
	}
	return db, nil
}

func rawSQLiteConn(ctx context.Context, db *sql.DB) (*sql.Conn, *sqlite3.SQLiteConn, error) {
	conn, err := db.Conn(ctx)
	if err != nil {
		return nil, nil, err
	}
	var sqliteConn *sqlite3.SQLiteConn
	if err := conn.Raw(func(driverConn any) error {
		c, ok := driverConn.(*sqlite3.SQLiteConn)
		if !ok {
			return fmt.Errorf("unexpected sqlite connection %T", driverConn)
		}
		sqliteConn = c
		return nil
	}); err != nil {
		conn.Close()
		return nil, nil, err
	}
	return conn, sqliteConn, nil
}

// pruneSnapshot drops every table outside the control allowlist, filters
// app_kv down to persist=1, and returns the snapshot's schema version.
func pruneSnapshot(ctx context.Context, path string) (int, error) {
	db, err := openSQLite(ctx, path)
	if err != nil {
		return 0, fmt.Errorf("open snapshot: %w", err)
	}
	defer db.Close()

	if _, err := db.ExecContext(ctx, "PRAGMA foreign_keys = OFF"); err != nil {
		return 0, err
	}
	rows, err := db.QueryContext(ctx,
		`SELECT name, type FROM sqlite_master WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'`)
	if err != nil {
		return 0, fmt.Errorf("list snapshot tables: %w", err)
	}
	var drops []string
	for rows.Next() {
		var name, typ string
		if err := rows.Scan(&name, &typ); err != nil {
			rows.Close()
			return 0, err
		}
		if controlTables[name] {
			continue
		}
		kind := "TABLE"
		if typ == "view" {
			kind = "VIEW"
		}
		drops = append(drops, "DROP "+kind+" IF EXISTS "+quoteIdent(name))
	}
	if err := rows.Err(); err != nil {
		rows.Close()
		return 0, err
	}
	rows.Close()

	for _, stmt := range drops {
		if _, err := db.ExecContext(ctx, stmt); err != nil {
			return 0, fmt.Errorf("prune snapshot: %w", err)
		}
	}
	if err := pruneNonPersistentKV(ctx, db); err != nil {
		return 0, err
	}
	if _, err := db.ExecContext(ctx, "VACUUM"); err != nil {
		return 0, fmt.Errorf("vacuum snapshot: %w", err)
	}
	var schemaVersion int
	if err := db.QueryRowContext(ctx, "PRAGMA user_version").Scan(&schemaVersion); err != nil {
		return 0, fmt.Errorf("read schema version: %w", err)
	}
	return schemaVersion, nil
}

func pruneNonPersistentKV(ctx context.Context, db *sql.DB) error {
	exists, err := tableExists(ctx, db, "app_kv")
	if err != nil || !exists {
		return err
	}
	hasPersist, err := tableHasColumn(ctx, db, "app_kv", "persist")
	if err != nil || !hasPersist {
		return err
	}
	_, err = db.ExecContext(ctx, "DELETE FROM app_kv WHERE COALESCE(persist, 0) <> 1")
	return err
}

func tableExists(ctx context.Context, db *sql.DB, name string) (bool, error) {
	var count int
	err := db.QueryRowContext(ctx,
		`SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = ?`, name).Scan(&count)
	return count > 0, err
}

func tableHasColumn(ctx context.Context, db *sql.DB, table, column string) (bool, error) {
	rows, err := db.QueryContext(ctx, "PRAGMA table_info("+quoteIdent(table)+")")
	if err != nil {
		return false, err
	}
	defer rows.Close()
	for rows.Next() {
		var cid int
		var name, typ string
		var notNull, pk int
		var dflt sql.NullString
		if err := rows.Scan(&cid, &name, &typ, &notNull, &dflt, &pk); err != nil {
			return false, err
		}
		if name == column {
			return true, nil
		}
	}
	return false, rows.Err()
}

func quoteIdent(name string) string {
	return `"` + strings.ReplaceAll(name, `"`, `""`) + `"`
}

func fileEntry(dir, name string) (FileEntry, error) {
	info, err := os.Stat(filepath.Join(dir, name))
	if err != nil {
		return FileEntry{}, err
	}
	sum, err := sha256File(filepath.Join(dir, name))
	if err != nil {
		return FileEntry{}, err
	}
	return FileEntry{Name: name, Size: info.Size(), SHA256: sum}, nil
}

func sha256File(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()
	h := sha256.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}

func verifyEntry(dir string, entry FileEntry) error {
	path := filepath.Join(dir, entry.Name)
	info, err := os.Stat(path)
	if err != nil {
		return fmt.Errorf("%w: %s: %v", ErrHashMismatch, entry.Name, err)
	}
	if info.Size() != entry.Size {
		return fmt.Errorf("%w: %s size %d, manifest %d",
			ErrHashMismatch, entry.Name, info.Size(), entry.Size)
	}
	sum, err := sha256File(path)
	if err != nil {
		return fmt.Errorf("%w: %s: %v", ErrHashMismatch, entry.Name, err)
	}
	if !strings.EqualFold(sum, entry.SHA256) {
		return fmt.Errorf("%w: %s", ErrHashMismatch, entry.Name)
	}
	return nil
}

func liveSchemaVersion(ctx context.Context, path string) (int, error) {
	if _, err := os.Stat(path); os.IsNotExist(err) {
		return 0, nil
	}
	db, err := openSQLite(ctx, path)
	if err != nil {
		return 0, fmt.Errorf("open live database: %w", err)
	}
	defer db.Close()
	var version int
	if err := db.QueryRowContext(ctx, "PRAGMA user_version").Scan(&version); err != nil {
		return 0, fmt.Errorf("read live schema version: %w", err)
	}
	return version, nil
}

func compareVersions(a, b string) int {
	pa, okA := parseVersion(a)
	pb, okB := parseVersion(b)
	if !okA || !okB {
		return strings.Compare(a, b)
	}
	for i := 0; i < len(pa) || i < len(pb); i++ {
		var x, y int
		if i < len(pa) {
			x = pa[i]
		}
		if i < len(pb) {
			y = pb[i]
		}
		if x != y {
			if x > y {
				return 1
			}
			return -1
		}
	}
	return 0
}

func parseVersion(v string) ([]int, bool) {
	v = strings.TrimPrefix(strings.TrimSpace(v), "v")
	if v == "" {
		return nil, false
	}
	parts := strings.Split(v, ".")
	out := make([]int, 0, len(parts))
	for _, part := range parts {
		n, err := strconv.Atoi(part)
		if err != nil {
			return nil, false
		}
		out = append(out, n)
	}
	return out, true
}
