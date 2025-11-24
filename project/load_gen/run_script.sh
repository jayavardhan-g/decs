#!/usr/bin/env bash
set -euo pipefail

# run_all.sh
# Usage: ./run_all.sh

# ----- Config -----
BASE_URL=${BASE_URL:-http://localhost:1234}
DURATION=${DURATION:-40s}
KEYSPACE=${KEYSPACE:-17000}
# VU list to run
VUS_LIST=(1 100 300 500 1000 1300 1700 2100 4000 7000)

declare -A WORKLOAD_SCRIPTS=(
  # ["get_only"]="get_only.js"
  # ["put_only"]="put_only.js"
  # ["delete_only"]="delete_only.js"
  # ["mixed"]="mixed.js"
  ["get_popular"]="get_popular.js"
)

# ----- Helpers -----
command_exists() {
  command -v "$1" >/dev/null 2>&1
}

# Check prerequisites
for cmd in k6 mpstat iostat jq; do
  if ! command_exists "$cmd"; then
    echo "ERROR: required command '$cmd' not found."
    exit 1
  fi
done

# Ensure scripts exist
for s in "${WORKLOAD_SCRIPTS[@]}"; do
  if [ ! -f "$s" ]; then
    echo "ERROR: missing workload script '$s'."
    exit 1
  fi
done

DURATION_SECS=$(echo "$DURATION" | tr -cd '0-9')

print_progress() {
  local pct=$1
  local label="$2"
  local width=30
  local filled=$(( (pct * width) / 100 ))
  if [ $filled -gt $width ]; then filled=$width; fi
  local empty=$((width - filled))
  local bar="$(printf '%0.s#' $(seq 1 $filled))$(printf '%0.s-' $(seq 1 $empty))"
  printf "\r%s |%s| %3d%%" "$label" "$bar" "$pct"
}

# ----- Main loop -----
echo "Starting benchmark suite..."
echo "VUs: ${VUS_LIST[*]}"
echo

for workload in "${!WORKLOAD_SCRIPTS[@]}"; do
  script=${WORKLOAD_SCRIPTS[$workload]}
  outcsv="results_${workload}.csv"
  
  # Initialize CSV Header
  echo "vus,tps,avg_ms,p50_ms,p90_ms,p95_ms,p99_ms,cpu_core0_max_pct,disk_dev_max_pct" > "$outcsv"

  for VUS in "${VUS_LIST[@]}"; do
    echo
    echo "-> Running workload=$workload | VUs=$VUS"

    # cleanup log files
    rm -f cpu_core0.log disk.log summary.json k6_run.log

    # 1. Start mpstat (background, no stdin)
    mpstat -P 0 1 > cpu_core0.log 2>&1 </dev/null & 
    MPSTAT_PID=$!

    # 2. Start iostat (background, no stdin)
    iostat -dx 1 > disk.log 2>&1 </dev/null & 
    IOSTAT_PID=$!

    # 3. Start k6
    export VUS BASE_URL KEYSPACE DURATION
    k6 run --quiet --summary-export=summary.json --env VUS="$VUS" --env BASE_URL="$BASE_URL" --env KEYSPACE="$KEYSPACE" --duration "${DURATION_SECS}s" "$script" > k6_run.log 2>&1 </dev/null &
    K6_PID=$!

    # Progress loop
    start_ts=$(date +%s)
    label="W:$workload V:$VUS"
    
    while kill -0 "$K6_PID" >/dev/null 2>&1; do
      now_ts=$(date +%s)
      elapsed=$(( now_ts - start_ts ))
      pct=$(( 100 * elapsed / DURATION_SECS ))
      if [ $pct -gt 100 ]; then pct=100; fi
      print_progress "$pct" "$label"
      sleep 1
    done
    print_progress 100 "$label"
    echo

    # 4. Wait ONLY for k6 to finish
    wait "$K6_PID" 2>/dev/null || true

    # 5. Kill stats collectors (AND DO NOT WAIT FOR THEM)
    kill "$MPSTAT_PID" 2>/dev/null || true
    kill "$IOSTAT_PID" 2>/dev/null || true
    
    # --- Data Extraction (Awk/Jq) ---

    # Calculate CPU Max
    CPU_MAX=$(awk 'BEGIN{max=0} 
      NR>2 && $0 !~ /Average/ { 
        # mpstat column 12 is usually %idle, check your specific mpstat version if this is wrong
        # usually: CPU %usr %nice %sys %iowait %irq %soft %steal %guest %gnice %idle
        idle=$NF; 
        if(idle ~ /^[0-9.]+$/) {
            util=100 - idle; 
            if(util>max) max=util; 
        }
      } 
      END{ printf("%.2f", max) }' cpu_core0.log 2>/dev/null || echo "0")

    # Calculate Disk Max
    DISK_MAX=$(awk 'BEGIN{max=0} 
      /^[a-z]/ { 
        # iostat last column is usually %util
        util=$NF; 
        if(util ~ /^[0-9.]+$/) {
            if(util+0 > max+0) max=util; 
        }
      } 
      END{ printf("%.2f", max) }' disk.log 2>/dev/null || echo "0")

    # Extract k6 metrics
    if [ -f summary.json ] && [ -s summary.json ]; then
      TPS=$(jq '.metrics.http_reqs.rate // 0' summary.json)
      AVG_MS=$(jq '.metrics.http_req_duration.avg // 0' summary.json)
      P50_MS=$(jq '.metrics.http_req_duration["p(50)"] // 0' summary.json)
      P90_MS=$(jq '.metrics.http_req_duration["p(90)"] // 0' summary.json)
      P95_MS=$(jq '.metrics.http_req_duration["p(95)"] // 0' summary.json)
      P99_MS=$(jq '.metrics.http_req_duration["p(99)"] // 0' summary.json)
    else
      TPS=0; AVG_MS=0; P50_MS=0; P90_MS=0; P95_MS=0; P99_MS=0
      echo "  ⚠️ Warning: summary.json missing or empty."
    fi

    # Append to CSV
    echo "${VUS},${TPS},${AVG_MS},${P50_MS},${P90_MS},${P95_MS},${P99_MS},${CPU_MAX},${DISK_MAX}" >> "$outcsv"
    echo "  Saved stats -> CPU:${CPU_MAX}% Disk:${DISK_MAX}% TPS:${TPS}"
    
    sleep 2
  done
done

echo "All workloads finished."