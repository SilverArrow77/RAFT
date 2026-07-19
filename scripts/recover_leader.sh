#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

rm -f data/wal-node*.log /tmp/leader-demo.log

./kv-store config0.txt > /tmp/node0.log 2>&1 &
p0=$!
./kv-store config1.txt > /tmp/node1.log 2>&1 &
p1=$!
./kv-store config2.txt > /tmp/node2.log 2>&1 &
p2=$!

sleep 2
for i in $(seq 1 5); do
  leader=$(grep -E 'became leader|heartbeat received' /tmp/node0.log /tmp/node1.log /tmp/node2.log | tail -n 5 || true)
  if [ -n "$leader" ]; then
    echo "iteration $i leader signal: $leader"
  fi
  kill "$p0" "$p1" "$p2" 2>/dev/null || true
  sleep 1
  ./kv-store config0.txt > /tmp/node0.log 2>&1 &
  p0=$!
  ./kv-store config1.txt > /tmp/node1.log 2>&1 &
  p1=$!
  ./kv-store config2.txt > /tmp/node2.log 2>&1 &
  p2=$!
  sleep 2
 done

wait "$p0" "$p1" "$p2" 2>/dev/null || true
