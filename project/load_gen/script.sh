#!/bin/bash
set -euo pipefail

# Usage: ./script.sh
# Ensure k6, vmstat (procps), iostat (sysstat), jq are installed.

BASE_URL=${BASE_URL:-http://localhost:1234}
DURATION=${DURATION:-20s}
KEYSPACE=${KEYSPACE:-20000}
# VU list (modify as you like)
VUS_LIST=(10 50 100 175 250 500 750 1000)
# VUS_LIST=(1)

# workloads and corresponding script filenames
declare -A WORKLOAD_SCRIPTS=(
  # ["get_only"]="get_only.js"
  # ["put_only"]="put_only.js"
  # ["delete_only"]="delete_only.js"
  ["mixed"]="mixed.js"
  ["get_popular"]="get_popular.js"
)

# ensure k6 scripts exist
for s in "${WORKLOAD_SCRIPTS[@]}"; do
  if [ ! -f "$s" ]; then
    echo "Missing $s in current directory. Aborting."
    exit 1
  fi
done

# loop workloads
for workload in "${!WORKLOAD_SCRIPTS[@]}"; do
  script=${WORKLOAD_SCRIPTS[$workload]}
  outcsv="results_${workload}.csv"
  echo "Running workload: $workload -> script: $script"
  echo "vus,tps,avg_ms,p50_ms,p90_ms,p95_ms,p99_ms,cpu_util_pct,disk_util_pct" > "$outcsv"

  for VUS in "${VUS_LIST[@]}"; do
    echo "  -> VUs = $VUS"

    # start vmstat and iostat
    vmstat 1 > cpu.log 2>&1 &
    CPU_PID=$!

    iostat -dx 1 > disk.log 2>&1 &
    DISK_PID=$!

    # run k6
    # pass BASE_URL, VUS, DURATION (duration without 's' is ok), KEYSPACE
    echo "    k6 run --summary-export=summary.json --env VUS=$VUS --env BASE_URL=$BASE_URL --env KEYSPACE=$KEYSPACE --env DURATION=$DURATION $script"
    k6 run --summary-export=summary.json --env VUS=$VUS --env BASE_URL=$BASE_URL --env KEYSPACE=$KEYSPACE --env DURATION=$DURATION "$script" > k6_run.log 2>&1 || true

    # stop monitor procs
    kill $CPU_PID $DISK_PID 2>/dev/null || true
    wait $CPU_PID 2>/dev/null || true
    wait $DISK_PID 2>/dev/null || true

    # compute CPU util (cpu.log format: vmstat has header lines; idle is column 15)
    CPU_IDLE_AVG=$(awk 'NR>2 {sum+=$15; count++} END {if(count>0) print sum/count; else print 0}' cpu.log)
    CPU_UTIL_AVG=$(awk -v idle="$CPU_IDLE_AVG" 'BEGIN {printf("%.2f", 100 - idle)}')

    # compute disk util (iostat -dx output, take %util column; heuristic: pick numeric lines)
    DISK_UTIL_AVG=$(awk '/^Device/ {next} /^[a-z]/ {sum+=$(NF); count++} END {if(count>0) printf("%.2f", sum/count); else print "0.00"}' disk.log)

    # parse summary.json (jq must be installed)
    if [ -f summary.json ]; then
      TPS=$(jq '.metrics.http_reqs.rate' summary.json)
      AVG_MS=$(jq '.metrics.http_req_duration.avg' summary.json)
      P50_MS=$(jq '.metrics.http_req_duration["p(50)"]' summary.json)
      P90_MS=$(jq '.metrics.http_req_duration["p(90)"]' summary.json)
      P95_MS=$(jq '.metrics.http_req_duration["p(95)"]' summary.json)
      P99_MS=$(jq '.metrics.http_req_duration["p(99)"]' summary.json)
    else
      TPS=0; AVG_MS=0; P50_MS=0; P90_MS=0; P95_MS=0; P99_MS=0
    fi

    # append to CSV
    echo "${VUS},${TPS},${AVG_MS},${P50_MS},${P90_MS},${P95_MS},${P99_MS},${CPU_UTIL_AVG},${DISK_UTIL_AVG}" >> "$outcsv"

    # small cooldown between runs
    sleep 3
  done

  echo "Saved results to $outcsv"
done

echo "All workloads complete."