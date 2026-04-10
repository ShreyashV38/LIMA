#!/bin/bash
# Fedora Linux File System & Vi Text Editor Benchmark
# Iterations: 1000

NUM_OPS=1000

# Create a temporary workspace
TEST_DIR=$(mktemp -d "/tmp/lima_fedora_bench.XXXXXX")
CLIP_DIR=$(mktemp -d "/tmp/lima_clip.XXXXXX")

get_time_ms() {
    local start=$1
    local end=$2
    awk -v s="$start" -v e="$end" 'BEGIN {printf "%8.2f", (e - s) / 1000000.0}'
}

echo "==============================================================="
echo "             Fedora File System & Vi - Benchmark"
echo "             Iterations: $NUM_OPS"
echo "==============================================================="

cd "$TEST_DIR" || exit 1

# 1. Create Directory (home/docs)
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    mkdir -p "home_$i/docs"
done
end=$(date +%s%N)
printf "Operation: create directory | Time: %s ms\n" "$(get_time_ms $start $end)"

# 2. Create File (home/file1.txt)
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    echo "Hello, World!" > "home_$i/file1.txt"
done
end=$(date +%s%N)
printf "Operation: create file      | Time: %s ms\n" "$(get_time_ms $start $end)"

# 3. Copy Directory
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    cp -r "home_$i" "$CLIP_DIR/home_clip"
    rm -rf "$CLIP_DIR/home_clip"
done
end=$(date +%s%N)
printf "Operation: copy directory   | Time: %s ms\n" "$(get_time_ms $start $end)"

# 4. Copy File
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    cp "home_$i/file1.txt" "$CLIP_DIR/file_clip.txt"
    rm "$CLIP_DIR/file_clip.txt"
done
end=$(date +%s%N)
printf "Operation: copy file        | Time: %s ms\n" "$(get_time_ms $start $end)"

# 5. Cut Directory
mkdir cut_dest_dir
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    mv "home_$i" "cut_dest_dir/home_$i"
done
end=$(date +%s%N)
printf "Operation: cut directory    | Time: %s ms\n" "$(get_time_ms $start $end)"

# 6. Cut File
mkdir cut_dest_file
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    mv "cut_dest_dir/home_$i/file1.txt" "cut_dest_file/file1_$i.txt"
done
end=$(date +%s%N)
printf "Operation: cut file         | Time: %s ms\n" "$(get_time_ms $start $end)"

# 7. Paste Directory
# To simulate paste separately, we copy once then paste 1000 times
cp -r "cut_dest_dir/home_1" "$CLIP_DIR/home_template"
mkdir paste_dir_bench
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    cp -r "$CLIP_DIR/home_template" "paste_dir_bench/home_$i"
done
end=$(date +%s%N)
printf "Operation: paste directory  | Time: %s ms\n" "$(get_time_ms $start $end)"

# 8. Paste File
cp "cut_dest_file/file1_1.txt" "$CLIP_DIR/file_template.txt"
mkdir paste_file_bench
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    cp "$CLIP_DIR/file_template.txt" "paste_file_bench/file_$i.txt"
done
end=$(date +%s%N)
printf "Operation: paste file       | Time: %s ms\n" "$(get_time_ms $start $end)"

# 9. Delete Directory
start=$(date +%s%N)
rm -rf "paste_dir_bench"
rm -rf "cut_dest_dir"
end=$(date +%s%N)
printf "Operation: delete directory | Time: %s ms\n" "$(get_time_ms $start $end)"

# 10. Delete File
start=$(date +%s%N)
rm -f paste_file_bench/*.txt
rm -f cut_dest_file/*.txt
end=$(date +%s%N)
printf "Operation: delete file      | Time: %s ms\n" "$(get_time_ms $start $end)"

# 11. Copy and Paste text in file (using vi)
echo "Hello, World!" > text_bench.txt
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    # vi command: yyp (yank line, paste)
    vi -e -s -c "norm yyp" -c "wq" text_bench.txt
done
end=$(date +%s%N)
printf "Operation: copy & paste text| Time: %s ms\n" "$(get_time_ms $start $end)"

# 12. Cut and Paste text in file (using vi)
echo "Hello, World!" > text_bench_cut.txt
start=$(date +%s%N)
for i in $(seq 1 $NUM_OPS); do
    # vi command: ddp (delete line, paste)
    vi -e -s -c "norm ddp" -c "wq" text_bench_cut.txt
done
end=$(date +%s%N)
printf "Operation: cut & paste text | Time: %s ms\n" "$(get_time_ms $start $end)"

# Cleanup
cd /tmp || exit 1
rm -rf "$TEST_DIR"
rm -rf "$CLIP_DIR"

echo "==============================================================="
