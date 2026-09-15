CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS settings (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS tasks (
    id                  TEXT PRIMARY KEY,
    title               TEXT NOT NULL,
    parent_id           TEXT REFERENCES tasks(id) ON DELETE CASCADE,
    sort_order          INTEGER NOT NULL DEFAULT 0,
    status              TEXT NOT NULL DEFAULT 'idle',
    active_since        TEXT,
    scheduled_start_at  TEXT,
    scheduled_end_at    TEXT,
    self_elapsed_ms     INTEGER NOT NULL DEFAULT 0,
    target_ms           INTEGER,
    target_reached_at   TEXT,
    created_at          TEXT NOT NULL,
    updated_at          TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS sessions (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id     TEXT NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    started_at  TEXT NOT NULL,
    duration_ms INTEGER NOT NULL,
    mode        TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS app_state (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_tasks_parent ON tasks(parent_id);
CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);
CREATE INDEX IF NOT EXISTS idx_tasks_scheduled_start ON tasks(scheduled_start_at);
CREATE INDEX IF NOT EXISTS idx_sessions_task ON sessions(task_id);
