package apptask

import (
	"database/sql"
	"path/filepath"
	"testing"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

const createTable = `CREATE TABLE app_task (
	id TEXT PRIMARY KEY,
	kind INTEGER NOT NULL,
	idempotency_key TEXT NOT NULL UNIQUE,
	state INTEGER NOT NULL,
	created_at INTEGER NOT NULL,
	updated_at INTEGER NOT NULL,
	result_json TEXT);`

func newLedger(t *testing.T) string {
	t.Helper()
	path := filepath.Join(t.TempDir(), "control.db")
	db, err := sql.Open("sqlite3", path)
	if err != nil {
		t.Fatalf("open: %v", err)
	}
	defer db.Close()
	if _, err := db.Exec(createTable); err != nil {
		t.Fatalf("create app_task: %v", err)
	}
	return path
}

func TestSubmitFinishAndReplay(t *testing.T) {
	path := newLedger(t)
	now := time.Date(2026, 11, 13, 8, 0, 0, 0, time.UTC)

	task, replayed, err := Submit(path, KindBackup, "backup-key-1", now)
	if err != nil {
		t.Fatalf("submit: %v", err)
	}
	if replayed {
		t.Fatal("first submit must not replay")
	}
	if task.State != StatePending {
		t.Fatalf("state = %d, want pending", task.State)
	}

	replay, replayed, err := Submit(path, KindBackup, "backup-key-1", now.Add(time.Hour))
	if err != nil {
		t.Fatalf("replay submit: %v", err)
	}
	if !replayed {
		t.Fatal("second submit within 24h must replay")
	}
	if replay.ID != task.ID {
		t.Fatalf("replay id = %q, want %q", replay.ID, task.ID)
	}

	if err := Finish(path, task.ID, StateSucceeded, `{"backup_path":"/tmp/b"}`, now.Add(2*time.Hour)); err != nil {
		t.Fatalf("finish: %v", err)
	}
	replay, replayed, err = Submit(path, KindBackup, "backup-key-1", now.Add(3*time.Hour))
	if err != nil {
		t.Fatalf("terminal replay submit: %v", err)
	}
	if !replayed || replay.State != StateSucceeded {
		t.Fatalf("terminal replay state = %d replayed = %v, want succeeded true", replay.State,
			replayed)
	}
	if replay.ResultJSON != `{"backup_path":"/tmp/b"}` {
		t.Fatalf("result = %q", replay.ResultJSON)
	}
	if err := Finish(path, task.ID, StateFailed, "late", now.Add(4*time.Hour)); err == nil {
		t.Fatal("finishing a terminal task must fail")
	}

	later := now.Add(25 * time.Hour)
	fresh, replayed, err := Submit(path, KindBackup, "backup-key-1", later)
	if err != nil {
		t.Fatalf("expired submit: %v", err)
	}
	if replayed {
		t.Fatal("a key older than 24h must start a new task")
	}
	if fresh.ID != task.ID || fresh.State != StatePending {
		t.Fatalf("fresh id/state = %q/%d", fresh.ID, fresh.State)
	}
	if fresh.CreatedAt != later.UnixMilli() {
		t.Fatalf("created_at = %d, want %d", fresh.CreatedAt, later.UnixMilli())
	}

	if _, _, err := Submit(path, KindBackup, "", now); err == nil {
		t.Fatal("empty key must be rejected")
	}
	if err := Finish(path, "missing", StateFailed, "", now); err == nil {
		t.Fatal("missing task must be reported")
	}
}
