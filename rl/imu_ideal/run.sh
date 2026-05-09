#!/usr/bin/env bash
# sesameRL training manager
#
# Usage:
#   ./run.sh start    start training (nohup) + TensorBoard
#   ./run.sh stop     stop both processes
#   ./run.sh status   show running state + last training output
#   ./run.sh clean    delete all logs and checkpoints (prompts)
#   ./run.sh logs     tail live training output

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TRAIN_PID_FILE="$SCRIPT_DIR/.train.pid"
TB_PID_FILE="$SCRIPT_DIR/.tb.pid"
TRAIN_LOG="$SCRIPT_DIR/train.log"
TB_LOG="$SCRIPT_DIR/tb.log"
LOG_DIR="$SCRIPT_DIR/logs"
TB_PORT=6006

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

is_running() {
    local pid_file="$1"
    [[ -f "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null
}

cmd_start() {
    cd "$SCRIPT_DIR"

    if is_running "$TRAIN_PID_FILE"; then
        echo -e "${YELLOW}Training already running (PID $(cat "$TRAIN_PID_FILE"))${NC}"
    else
        mkdir -p "$LOG_DIR"
        nohup uv run train.py --nan-guard > "$TRAIN_LOG" 2>&1 &
        echo $! > "$TRAIN_PID_FILE"
        echo -e "${GREEN}✓ Training started${NC}  PID=$(cat "$TRAIN_PID_FILE")"
        echo -e "  Follow output:  ${CYAN}./run.sh logs${NC}"
    fi

    if is_running "$TB_PID_FILE"; then
        echo -e "${YELLOW}TensorBoard already running (PID $(cat "$TB_PID_FILE"))${NC}"
    else
        mkdir -p "$LOG_DIR"
        nohup uv run tensorboard --logdir "$LOG_DIR" --port $TB_PORT > "$TB_LOG" 2>&1 &
        echo $! > "$TB_PID_FILE"
        echo -e "${GREEN}✓ TensorBoard started${NC} PID=$(cat "$TB_PID_FILE")"
        echo -e "  Dashboard:      ${CYAN}http://localhost:$TB_PORT${NC}"
    fi
}

cmd_stop() {
    local stopped=0

    if is_running "$TRAIN_PID_FILE"; then
        local pid
        pid=$(cat "$TRAIN_PID_FILE")
        kill "$pid" && echo -e "${GREEN}✓ Training stopped${NC}  (PID $pid)" || \
            echo -e "${RED}Failed to stop training (PID $pid)${NC}"
        stopped=1
    else
        echo -e "${YELLOW}Training not running${NC}"
    fi
    rm -f "$TRAIN_PID_FILE"

    if is_running "$TB_PID_FILE"; then
        local pid
        pid=$(cat "$TB_PID_FILE")
        kill "$pid" && echo -e "${GREEN}✓ TensorBoard stopped${NC} (PID $pid)" || \
            echo -e "${RED}Failed to stop TensorBoard (PID $pid)${NC}"
        stopped=1
    else
        echo -e "${YELLOW}TensorBoard not running${NC}"
    fi
    rm -f "$TB_PID_FILE"

    [[ $stopped -eq 0 ]] && echo "Nothing was running."
}

cmd_status() {
    echo "━━━  sesameRL status  ━━━"

    if is_running "$TRAIN_PID_FILE"; then
        local pid
        pid=$(cat "$TRAIN_PID_FILE")
        echo -e "Training:    ${GREEN}running${NC} (PID $pid)"
    else
        echo -e "Training:    ${RED}stopped${NC}"
    fi

    if is_running "$TB_PID_FILE"; then
        local pid
        pid=$(cat "$TB_PID_FILE")
        echo -e "TensorBoard: ${GREEN}running${NC} (PID $pid)  →  http://localhost:$TB_PORT"
    else
        echo -e "TensorBoard: ${RED}stopped${NC}"
    fi

    if [[ -d "$LOG_DIR" ]]; then
        local n_runs
        n_runs=$(find "$LOG_DIR" -mindepth 1 -maxdepth 1 -type d | wc -l)
        local latest
        latest=$(ls -dt "$LOG_DIR"/*/ 2>/dev/null | head -1 | xargs basename 2>/dev/null || echo "none")
        local n_ckpt
        n_ckpt=$(find "$LOG_DIR" -name "model_*.pt" | wc -l)
        echo ""
        echo "Runs:        $n_runs  (latest: $latest)"
        echo "Checkpoints: $n_ckpt"
    fi

    if [[ -f "$TRAIN_LOG" ]]; then
        echo ""
        echo "━━━  last training output  ━━━"
        tail -20 "$TRAIN_LOG"
    fi
}

cmd_logs() {
    if [[ ! -f "$TRAIN_LOG" ]]; then
        echo -e "${YELLOW}No training log found. Run ./run.sh start first.${NC}"
        exit 1
    fi
    echo -e "${CYAN}Following $TRAIN_LOG  (Ctrl+C to stop)${NC}"
    tail -f "$TRAIN_LOG"
}

cmd_clean() {
    local has_content=0

    if [[ -d "$LOG_DIR" ]]; then
        has_content=1
        echo -e "${YELLOW}Will delete all checkpoints and TensorBoard logs:${NC}"
        echo "  $LOG_DIR"
        echo "  $(du -sh "$LOG_DIR" 2>/dev/null | cut -f1) used"
    fi
    if [[ -f "$TRAIN_LOG" || -f "$TB_LOG" ]]; then
        has_content=1
        echo -e "${YELLOW}Will delete process logs:${NC}"
        [[ -f "$TRAIN_LOG" ]] && echo "  $TRAIN_LOG"
        [[ -f "$TB_LOG"    ]] && echo "  $TB_LOG"
    fi

    if [[ $has_content -eq 0 ]]; then
        echo "Nothing to clean."
        return
    fi

    if is_running "$TRAIN_PID_FILE"; then
        echo -e "${RED}Training is still running — stop it first with ./run.sh stop${NC}"
        exit 1
    fi

    echo ""
    read -r -p "Delete everything? [y/N] " confirm
    case "$confirm" in
        [yY])
            rm -rf "$LOG_DIR"
            rm -f "$TRAIN_LOG" "$TB_LOG" "$TRAIN_PID_FILE" "$TB_PID_FILE"
            echo -e "${GREEN}✓ Cleaned${NC}"
            ;;
        *)
            echo "Cancelled."
            ;;
    esac
}

case "${1:-}" in
    start)  cmd_start  ;;
    stop)   cmd_stop   ;;
    status) cmd_status ;;
    logs)   cmd_logs   ;;
    clean)  cmd_clean  ;;
    *)
        echo "Usage: $0 {start|stop|status|logs|clean}"
        echo ""
        echo "  start   Start training (nohup) + TensorBoard in background"
        echo "  stop    Stop both processes"
        echo "  status  Show running state, checkpoint count, last output"
        echo "  logs    Tail live training output (Ctrl+C to exit)"
        echo "  clean   Delete all logs and checkpoints (prompts for confirmation)"
        exit 1
        ;;
esac
