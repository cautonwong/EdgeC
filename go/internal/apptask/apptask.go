// Package apptask records install, upgrade, and backup operations in the
// control database's app_task ledger. Times are epoch milliseconds, keys replay
// for 24 hours, and terminal rows never change again, matching
// modules/edge_mgmt/src/app_task.c.
package apptask

import (
	"database/sql"
	"errors"
	"fmt"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

// Kind mirrors edge_app_task_kind_t.
type Kind int

const (
	KindInstall Kind = 1
	KindUpgrade Kind = 2
	KindBackup  Kind = 3
)

// State mirrors edge_app_task_state_t.
type State int

const (
	StatePending   State = 0
	StateRunning   State = 1
	StateSucceeded State = 2
	StateFailed    State = 3
)

const (
	idempotencyWindow = 24 * time.Hour
	keyMaxSize        = 128
	resultMaxSize     = 4096
)

// Task is one app_task row.
type Task struct {
	ID             string
	Kind           Kind
	IdempotencyKey string
	State          State
	CreatedAt      int64
	UpdatedAt      int64
	ResultJSON     string
}

// taskID is the FNV-1a hash of the key, matching the C host layer so both
// writers address the same row for a key.
func taskID(key string) string {
	hash := uint64(14695981039346656037)
	for i := 0; i < len(key); i++ {
		hash ^= uint64(key[i])
		hash *= 1099511628211
	}
	return fmt.Sprintf("task-%016x", hash)
}

func openDB(dbPath string) (*sql.DB, error) {
	if dbPath == "" {
		return nil, errors.New("apptask: database path is required")
	}
	return sql.Open("sqlite3", dbPath)
}

// Submit creates a PENDING task, or returns the existing row for a key created
// within the last 24 hours with replayed set.
func Submit(dbPath string, kind Kind, idempotencyKey string, now time.Time) (Task, bool, error) {
	var zero Task
	if kind != KindInstall && kind != KindUpgrade && kind != KindBackup {
		return zero, false, errors.New("apptask: unknown kind")
	}
	if idempotencyKey == "" || len(idempotencyKey) >= keyMaxSize {
		return zero, false, errors.New("apptask: invalid idempotency key")
	}

	db, err := openDB(dbPath)
	if err != nil {
		return zero, false, err
	}
	defer db.Close()

	var existing Task
	err = db.QueryRow(
		"SELECT id, kind, idempotency_key, state, created_at, updated_at, "+
			"COALESCE(result_json, '') FROM app_task WHERE idempotency_key = ?",
		idempotencyKey,
	).Scan(&existing.ID, &existing.Kind, &existing.IdempotencyKey, &existing.State,
		&existing.CreatedAt, &existing.UpdatedAt, &existing.ResultJSON)
	if err == nil {
		if now.UnixMilli()-existing.CreatedAt <= idempotencyWindow.Milliseconds() {
			return existing, true, nil
		}
	} else if !errors.Is(err, sql.ErrNoRows) {
		return zero, false, err
	}

	nowMs := now.UnixMilli()
	task := Task{
		ID:             taskID(idempotencyKey),
		Kind:           kind,
		IdempotencyKey: idempotencyKey,
		State:          StatePending,
		CreatedAt:      nowMs,
		UpdatedAt:      nowMs,
	}
	_, err = db.Exec(
		"INSERT OR REPLACE INTO app_task "+
			"(id, kind, idempotency_key, state, created_at, updated_at, result_json) "+
			"VALUES (?, ?, ?, ?, ?, ?, NULL)",
		task.ID, task.Kind, task.IdempotencyKey, task.State, task.CreatedAt, task.UpdatedAt)
	if err != nil {
		return zero, false, err
	}
	return task, false, nil
}

// Finish moves a non-terminal task to RUNNING, SUCCEEDED, or FAILED. resultJSON
// may be empty and must fit in the 4096-byte column.
func Finish(dbPath, id string, state State, resultJSON string, now time.Time) error {
	if id == "" {
		return errors.New("apptask: task id is required")
	}
	if state != StateRunning && state != StateSucceeded && state != StateFailed {
		return errors.New("apptask: invalid target state")
	}
	if len(resultJSON) >= resultMaxSize {
		return errors.New("apptask: result too large")
	}

	db, err := openDB(dbPath)
	if err != nil {
		return err
	}
	defer db.Close()

	var current State
	err = db.QueryRow("SELECT state FROM app_task WHERE id = ?", id).Scan(&current)
	if errors.Is(err, sql.ErrNoRows) {
		return fmt.Errorf("apptask: task %s not found", id)
	}
	if err != nil {
		return err
	}
	if current == StateSucceeded || current == StateFailed {
		return fmt.Errorf("apptask: task %s is already terminal", id)
	}

	var result any
	if resultJSON != "" {
		result = resultJSON
	}
	_, err = db.Exec(
		"UPDATE app_task SET state = ?, updated_at = ?, result_json = ? WHERE id = ?",
		state, now.UnixMilli(), result, id)
	return err
}
