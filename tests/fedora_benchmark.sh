#!/bin/bash
# Fedora Linux File System & Vi Text Editor Benchmark
# Compares standard Linux / POSIX I/O with the LIMA operations.

# Note: Operations are scaled down compared to purely in-memory LIMA bench 
# because fork/exec overhead of creating OS processes is significantly higher.
NUM_OPS=1000
NUM_VI_OPS=100  # vi starts a whole editor process, so we keep this lower

# Create a temporary workspace safely
TEST_DIR=$(mktemp -d "/tmp/lima_fedora_bench.XXXXXX")

# Helper for timing (returns milliseconds)
get_time_ms() {
    local start=$1
    local end=$2
    # Convert GNU date nanoseconds to milliseconds
    awk -v s="$start" -v e="$end" 'BEGIN {printf "%8.2f", (e - s) / 1000000.0}'
}

echo "==============================================================="
echo "             Fedora File System & Vi - Benchmark"
echo "==============================================================="

cd "$TEST_DIR" || exit 1

# 1. mkdir
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    mkdir "dir_$i"
done
end=$(date +%s%N)
printf "Operation: mkdir       | Iterations: %-6d | Time: %s ms\n" $NUM_OPS "$(get_time_ms $start $end)"

# 2. cd & pwd
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    cd "dir_$i" || continue
    pwd > /dev/null
    cd ..
done
end=$(date +%s%N)
printf "Operation: cd & pwd    | Iterations: %-6d | Time: %s ms\n" $NUM_OPS "$(get_time_ms $start $end)"

# 3. touch
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    touch "file_$i.txt"
done
end=$(date +%s%N)
printf "Operation: touch       | Iterations: %-6d | Time: %s ms\n" $NUM_OPS "$(get_time_ms $start $end)"

# 4. vi editor (write)
start=$(date +%s%N)
for i in $(seq 1 $NUM_VI_OPS); do
    # -e: Ex mode, -s: silent (no interactive prompt). 
    # This runs the actual vi binary to insert text headless.
    vi -e -s -c "norm iHello, Fedora benchmark!" -c "wq" "file_$i.txt"
done
end=$(date +%s%N)
printf "Operation: vi write    | Iterations: %-6d | Time: %s ms\n" $NUM_VI_OPS "$(get_time_ms $start $end)"

# 5. cp (clipboard copy/paste analog)
mkdir paste_target
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    cp "file_$i.txt" "paste_target/clipfile_$i.txt"
done
end=$(date +%s%N)
printf "Operation: cp (paste)  | Iterations: %-6d | Time: %s ms\n" $NUM_OPS "$(get_time_ms $start $end)"

# 6. pushd & popd (session navigation analog)
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    pushd "dir_$i" > /dev/null
    popd > /dev/null
done
end=$(date +%s%N)
printf "Operation: pushd/popd  | Iterations: %-6d | Time: %s ms\n" $NUM_OPS "$(get_time_ms $start $end)"

# 7. tar archive (persistence analog)
start=$(date +%s%N)
tar -cf backup.tar *
end=$(date +%s%N)
printf "Operation: tar (save)  | Nodes:      %-6d | Time: %s ms\n" $((NUM_OPS * 2)) "$(get_time_ms $start $end)"

start=$(date +%s%N)
tar -xf backup.tar
end=$(date +%s%N)
printf "Operation: tar (load)  | Nodes:      %-6d | Time: %s ms\n" $((NUM_OPS * 2)) "$(get_time_ms $start $end)"

# 8. rm (delete)
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    rm -rf "dir_$i" "file_$i.txt" "paste_target/clipfile_$i.txt"
done
end=$(date +%s%N)
printf "Operation: rm (delete) | Iterations: %-6d | Time: %s ms\n" $((NUM_OPS * 3)) "$(get_time_ms $start $end)"

# Cleanup
cd /tmp || exit 1
rm -rf "$TEST_DIR"

echo "==============================================================="
