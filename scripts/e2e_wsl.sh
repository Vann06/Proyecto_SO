#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_FILE="$ROOT_DIR/e2e_report.txt"
TMP_DIR="$(mktemp -d)"
SERVER_PID=""
PASS_COUNT=0
FAIL_COUNT=0
CASE_PORTS=(9301 9302 9303 9304 9305)

BACKUP_ENV="$TMP_DIR/env.backup"
if [ -f .env ]; then
    cp .env "$BACKUP_ENV"
fi

cleanup() {
    stop_server_pid

    for port in "${CASE_PORTS[@]}"; do
        kill_listeners_on_port "$port"
        wait_port_free "$port" || true
    done

    if [ -f "$BACKUP_ENV" ]; then
        cp "$BACKUP_ENV" .env
    else
        rm -f .env
    fi

    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

stop_server_pid() {
    if [ -n "${SERVER_PID}" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    SERVER_PID=""
}

pids_on_port() {
    local port="$1"

    if command -v lsof >/dev/null 2>&1; then
        lsof -t -iTCP:"${port}" -sTCP:LISTEN 2>/dev/null | sort -u
        return 0
    fi

    if command -v fuser >/dev/null 2>&1; then
        fuser -n tcp "${port}" 2>/dev/null | tr ' ' '\n' | sed '/^$/d' | sort -u
        return 0
    fi

    return 0
}

is_port_listening() {
    local port="$1"
    bash -lc "exec 3<>/dev/tcp/127.0.0.1/${port}" >/dev/null 2>&1
}

kill_listeners_on_port() {
    local port="$1"
    local pids

    pids="$(pids_on_port "$port" || true)"
    if [ -z "$pids" ]; then
        return 0
    fi

    while read -r pid; do
        [ -n "$pid" ] || continue
        kill "$pid" 2>/dev/null || true
    done <<< "$pids"

    for _ in $(seq 1 20); do
        if ! is_port_listening "$port"; then
            return 0
        fi
        sleep 0.1
    done

    pids="$(pids_on_port "$port" || true)"
    while read -r pid; do
        [ -n "$pid" ] || continue
        kill -9 "$pid" 2>/dev/null || true
    done <<< "$pids"

    return 0
}

wait_port_free() {
    local port="$1"
    for _ in $(seq 1 50); do
        if ! is_port_listening "$port"; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

log_line() {
    echo "$1" | tee -a "$REPORT_FILE"
}

start_server() {
    local port="$1"
    local mode="$2"
    local inactivity="$3"
    local enforce_override="${4:-}"

    stop_server_pid
    kill_listeners_on_port "$port"
    wait_port_free "$port" || true

    {
        echo "CHAT_ENV=${mode}"
        echo "CHAT_INACTIVITY_TIMEOUT=${inactivity}"
        if [ -n "$enforce_override" ]; then
            echo "CHAT_ENFORCE_UNIQUE_IP=${enforce_override}"
        fi
    } > .env

    if command -v stdbuf >/dev/null 2>&1; then
        stdbuf -oL ./server "$port" > "$TMP_DIR/server_${port}.log" 2>&1 &
    else
        ./server "$port" > "$TMP_DIR/server_${port}.log" 2>&1 &
    fi
    SERVER_PID="$!"

    for _ in $(seq 1 200); do
        if ! kill -0 "$SERVER_PID" 2>/dev/null; then
            cat "$TMP_DIR/server_${port}.log" >> "$REPORT_FILE"
            return 1
        fi

        if is_port_listening "$port"; then
            return 0
        fi

        sleep 0.1
    done

    cat "$TMP_DIR/server_${port}.log" >> "$REPORT_FILE"
    return 1
}

assert_contains() {
    local file="$1"
    local expected="$2"
    grep -Fq "$expected" "$file"
}

assert_not_contains() {
    local file="$1"
    local unexpected="$2"
    ! grep -Fq "$unexpected" "$file"
}

run_case() {
    local case_name="$1"
    shift

    log_line ""
    log_line "[CASE] ${case_name}"

    if "$@"; then
        log_line "[PASS] ${case_name}"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        log_line "[FAIL] ${case_name}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

dump_case_logs() {
    local title="$1"
    shift
    log_line "--- ${title} ---"
    for file in "$@"; do
        if [ -f "$file" ]; then
            log_line "file: $file"
            cat "$file" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
        fi
    done
}

case_duplicate_ip_production() {
    local port=9301
    local a_log="$TMP_DIR/case1_alice.log"
    local b_log="$TMP_DIR/case1_bob.log"

    start_server "$port" production 30 || return 1

    ({ sleep 5; printf 'EXIT\n'; } | ./client alice 127.0.0.1 "$port" > "$a_log" 2>&1) &
    local p1=$!
    sleep 2
    printf 'EXIT\n' | ./client bob 127.0.0.1 "$port" > "$b_log" 2>&1 || true
    wait "$p1" || true

    dump_case_logs "case1 production" "$a_log" "$b_log" "$TMP_DIR/server_${port}.log"

    assert_contains "$a_log" "Registro exitoso" || return 1
    assert_contains "$b_log" "Error en registro: ip duplicada" || return 1
    return 0
}

case_allow_same_ip_testing() {
    local port=9302
    local a_log="$TMP_DIR/case2_alice.log"
    local b_log="$TMP_DIR/case2_bob.log"

    start_server "$port" testing 30 || return 1

    ({ sleep 5; printf 'EXIT\n'; } | ./client alice 127.0.0.1 "$port" > "$a_log" 2>&1) &
    local p1=$!
    sleep 2
    printf 'EXIT\n' | ./client bob 127.0.0.1 "$port" > "$b_log" 2>&1 || true
    wait "$p1" || true

    dump_case_logs "case2 testing" "$a_log" "$b_log" "$TMP_DIR/server_${port}.log"

    assert_contains "$a_log" "Registro exitoso" || return 1
    assert_contains "$b_log" "Registro exitoso" || return 1
    return 0
}

case_core_flow_and_timeout() {
    local port=9303
    local alice_log="$TMP_DIR/case3_alice.log"
    local bob_log="$TMP_DIR/case3_bob.log"
    local charlie_log="$TMP_DIR/case3_charlie.log"

    start_server "$port" testing 5 || return 1

    ({ sleep 20; printf 'EXIT\n'; } | ./client bob 127.0.0.1 "$port" > "$bob_log" 2>&1) &
    local p_bob=$!

    ({ sleep 3; printf 'DM bob dm_ok\n'; sleep 2; printf 'BROADCAST hola_todos\n'; sleep 2; printf 'STATUS OCUPADO\n'; sleep 12; printf 'EXIT\n'; } | ./client alice 127.0.0.1 "$port" > "$alice_log" 2>&1) &
    local p_alice=$!

    ({
        sleep 12
        printf 'LIST\n'
        sleep 1
        printf 'INFO alice\n'
        sleep 1
        printf 'INFO bob\n'
        sleep 1
        printf 'EXIT\n'
    } | ./client charlie 127.0.0.1 "$port" > "$charlie_log" 2>&1) &
    local p_charlie=$!

    wait "$p_alice" || true
    wait "$p_charlie" || true
    wait "$p_bob" || true

    dump_case_logs "case3 core" "$alice_log" "$bob_log" "$charlie_log" "$TMP_DIR/server_${port}.log"

    assert_contains "$bob_log" "[General - alice]: hola_todos" || return 1
    assert_contains "$charlie_log" "alice,127.0.0.1,INACTIVO" || return 1
    assert_contains "$charlie_log" "bob,127.0.0.1,INACTIVO" || return 1
    assert_not_contains "$alice_log" "Error de protocolo" || return 1
    assert_not_contains "$bob_log" "Error de protocolo" || return 1
    return 0
}

case_exit_cleanup() {
    local port=9304
    local alice_log="$TMP_DIR/case4_alice.log"
    local bob_log="$TMP_DIR/case4_bob.log"

    start_server "$port" testing 30 || return 1

    ({ sleep 1; printf 'EXIT\n'; } | ./client alice 127.0.0.1 "$port" > "$alice_log" 2>&1) &
    local p_alice=$!
    wait "$p_alice" || true

    ({ sleep 4; printf 'LIST\n'; sleep 1; printf 'EXIT\n'; } | ./client bob 127.0.0.1 "$port" > "$bob_log" 2>&1) || true

    dump_case_logs "case4 cleanup" "$alice_log" "$bob_log" "$TMP_DIR/server_${port}.log"

    assert_contains "$bob_log" "[Servidor Info]:" || return 1
    assert_not_contains "$bob_log" "alice" || return 1
    return 0
}

case_stress_basic() {
    local port=9305
    local observer_log="$TMP_DIR/case5_observer.log"

    start_server "$port" testing 30 || return 1

    ({ sleep 10; printf 'EXIT\n'; } | ./client observer 127.0.0.1 "$port" > "$observer_log" 2>&1) &
    local p_observer=$!

    sleep 1

    local pids=()
    for user in u1 u2 u3 u4 u5; do
        ({ sleep 2; printf 'BROADCAST ping_%s\n' "$user"; sleep 2; printf 'EXIT\n'; } | ./client "$user" 127.0.0.1 "$port" > "$TMP_DIR/case5_${user}.log" 2>&1) &
        pids+=("$!")
    done

    for pid in "${pids[@]}"; do
        wait "$pid" || true
    done
    wait "$p_observer" || true

    dump_case_logs "case5 stress" "$observer_log" "$TMP_DIR/server_${port}.log" "$TMP_DIR/case5_u1.log" "$TMP_DIR/case5_u2.log" "$TMP_DIR/case5_u3.log" "$TMP_DIR/case5_u4.log" "$TMP_DIR/case5_u5.log"

    assert_contains "$observer_log" "[General -" || return 1

    for user in u1 u2 u3 u4 u5; do
        assert_contains "$TMP_DIR/case5_${user}.log" "Conexión cerrada. Hasta luego!" || return 1
        assert_not_contains "$TMP_DIR/case5_${user}.log" "Error en registro" || return 1
    done

    return 0
}

: > "$REPORT_FILE"
log_line "=== e2e report ==="
log_line "root: $ROOT_DIR"

if ! make clean >> "$REPORT_FILE" 2>&1; then
    log_line "[FAIL] make clean"
    exit 1
fi

if ! make all >> "$REPORT_FILE" 2>&1; then
    log_line "[FAIL] make all"
    exit 1
fi

run_case "production bloquea ip duplicada" case_duplicate_ip_production
run_case "testing permite misma ip" case_allow_same_ip_testing
run_case "flujo core con timeout inactivo" case_core_flow_and_timeout
run_case "exit limpia sesion" case_exit_cleanup
run_case "estres basico concurrente" case_stress_basic

log_line ""
log_line "=== summary ==="
log_line "pass: ${PASS_COUNT}"
log_line "fail: ${FAIL_COUNT}"

if [ "$FAIL_COUNT" -ne 0 ]; then
    exit 1
fi

exit 0
