#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/vfs/vfs.h"
#include "../include/core/clipboard.h"
#include "../include/data_structures/gap_buffer.h"
#include "../include/data_structures/select_buffer.h"

// Helper to calculate time in milliseconds
static double get_time_ms(clock_t start, clock_t end) {
    return ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
}

static void run_benchmarks(int iterations) {
    Vfs *vfs = vfs_create();
    VfsClipboard *clip = vfs_clipboard_create();
    SelectBuffer *sb = sb_create();
    clock_t start, end;

    printf("===============================================================\n");
    printf("             LIMA Virtual File System - Benchmark\n");
    printf("             Iterations: %-6d\n", iterations);
    printf("===============================================================\n");

    // 1. Create Directory (home, docs)
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char name[64];
        snprintf(name, sizeof(name), "home_%d", i);
        vfs_mkdir(vfs, name);
        vfs_cd(vfs, name);
        vfs_mkdir(vfs, "docs");
        vfs_cd(vfs, "/");
    }
    end = clock();
    printf("Operation: create directory | Time: %8.2f ms\n", get_time_ms(start, end));

    // 2. Create File (file1.txt with content)
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/home_%d/file1.txt", i);
        vfs_touch(vfs, path);
        GapBuffer *gb = vfs_open_file(vfs, path);
        if (gb) {
            gb_insert(gb, "Hello, World!", 13);
        }
    }
    end = clock();
    printf("Operation: create file      | Time: %8.2f ms\n", get_time_ms(start, end));

    // 3. Copy Directory
    start = clock();
    for (int i = 0; i < iterations; i++) {
        vfs_cd(vfs, "/");
        char name[64];
        snprintf(name, sizeof(name), "home_%d", i);
        vfs_clipboard_copy(clip, vfs, name);
        vfs_clipboard_clear(clip);
    }
    end = clock();
    printf("Operation: copy directory   | Time: %8.2f ms\n", get_time_ms(start, end));

    // 4. Copy File
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/home_%d", i);
        vfs_cd(vfs, path);
        vfs_clipboard_copy(clip, vfs, "file1.txt");
        vfs_clipboard_clear(clip);
    }
    end = clock();
    printf("Operation: copy file        | Time: %8.2f ms\n", get_time_ms(start, end));

    // 5. Cut Directory
    // We'll cut each home_i and paste it into a new dest to keep it in the tree
    vfs_mkdir(vfs, "/cut_dest_dir");
    start = clock();
    for (int i = 0; i < iterations; i++) {
        vfs_cd(vfs, "/");
        char name[64];
        snprintf(name, sizeof(name), "home_%d", i);
        vfs_clipboard_cut(clip, vfs, name);
        
        vfs_cd(vfs, "/cut_dest_dir");
        vfs_clipboard_paste(clip, vfs);
    }
    end = clock();
    printf("Operation: cut directory    | Time: %8.2f ms\n", get_time_ms(start, end));
    // Note: paste is included in cut benchmark to keep it functional, 
    // but the user also asked for separate paste benchmarks. 
    // We'll follow the exact request by doing them separately if possible, 
    // but cut requires paste to be "pasted" or it's just a delete.

    // Re-create some for cut file
    vfs_mkdir(vfs, "/cut_dest_file");
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/cut_dest_dir/home_%d", i);
        vfs_cd(vfs, path);
        vfs_clipboard_cut(clip, vfs, "file1.txt");
        
        vfs_cd(vfs, "/cut_dest_file");
        // We need unique names for multiple pastes of "file1.txt" into same dir
        // Or just move it back. Let's move it to cur_dest_file / file_i
        // But clipboard uses the original name.
        // So we'll paste into unique directories.
        char dest_sub[64];
        snprintf(dest_sub, sizeof(dest_sub), "dest_%d", i);
        vfs_mkdir(vfs, dest_sub);
        vfs_cd(vfs, dest_sub);
        vfs_clipboard_paste(clip, vfs);
    }
    end = clock();
    printf("Operation: cut file         | Time: %8.2f ms\n", get_time_ms(start, end));

    // 7. Paste Directory
    // We already did some pasting. Let's do 1000 pastes from a single copy.
    vfs_cd(vfs, "/cut_dest_dir");
    vfs_clipboard_copy(clip, vfs, "home_0"); // Copying home_0
    vfs_mkdir(vfs, "/paste_dir_bench");
    vfs_cd(vfs, "/paste_dir_bench");
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char sub[32];
        snprintf(sub, sizeof(sub), "p_%d", i);
        vfs_mkdir(vfs, sub);
        vfs_cd(vfs, sub);
        vfs_clipboard_paste(clip, vfs);
        vfs_cd(vfs, "..");
    }
    end = clock();
    printf("Operation: paste directory  | Time: %8.2f ms\n", get_time_ms(start, end));

    // 8. Paste File
    vfs_cd(vfs, "/cut_dest_file/dest_0");
    vfs_clipboard_copy(clip, vfs, "file1.txt");
    vfs_mkdir(vfs, "/paste_file_bench");
    vfs_cd(vfs, "/paste_file_bench");
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char sub[32];
        snprintf(sub, sizeof(sub), "pf_%d", i);
        vfs_mkdir(vfs, sub);
        vfs_cd(vfs, sub);
        vfs_clipboard_paste(clip, vfs);
        vfs_cd(vfs, "..");
    }
    end = clock();
    printf("Operation: paste file       | Time: %8.2f ms\n", get_time_ms(start, end));

    // 9. Delete Directory
    start = clock();
    vfs_rm(vfs, "/paste_dir_bench");
    vfs_rm(vfs, "/cut_dest_dir");
    end = clock();
    printf("Operation: delete directory | Time: %8.2f ms\n", get_time_ms(start, end));

    // 10. Delete File
    vfs_cd(vfs, "/cut_dest_file");
    start = clock();
    for (int i = 0; i < iterations; i++) {
        char sub[32];
        snprintf(sub, sizeof(sub), "dest_%d", i);
        vfs_cd(vfs, sub);
        vfs_rm(vfs, "file1.txt");
        vfs_cd(vfs, "..");
    }
    end = clock();
    printf("Operation: delete file      | Time: %8.2f ms\n", get_time_ms(start, end));

    // 11. Copy and Paste text in file
    vfs_cd(vfs, "/");
    vfs_touch(vfs, "test_text.txt");
    GapBuffer *gb = vfs_open_file(vfs, "test_text.txt");
    
    // Initial line
    gb_insert(gb, "Hello, World!\n", 14);
    
    start = clock();
    for (int i = 0; i < iterations; i++) {
        // Mimic 'yyp' by only storing a single line in the clipboard buffer 
        // rather than grabbing the exponentially growing whole file.
        char line_to_yank[] = "Hello, World!\n";
        sb_store(sb, line_to_yank, 14);
        
        size_t slen;
        const char *stext = sb_get_text(sb, &slen); // "Paste"
        gb_insert(gb, stext, slen);
    }
    end = clock();
    printf("Operation: copy & paste text| Time: %8.2f ms\n", get_time_ms(start, end));

    // 12. Cut and Paste text in file
    // Clear it first
    gb_move_cursor(gb, 0);
    gb_delete_after(gb, gb_size(gb));
    gb_insert(gb, "Hello, World!", 13);
    start = clock();
    for (int i = 0; i < iterations; i++) {
        size_t len = gb_size(gb);
        char *text = gb_get_text(gb, &len);
        sb_store(sb, text, len);
        gb_move_cursor(gb, 0);
        gb_delete_after(gb, len); // "Cut"
        free(text);

        size_t slen;
        const char *stext = sb_get_text(sb, &slen); // "Paste"
        gb_insert(gb, stext, slen);
    }
    end = clock();
    printf("Operation: cut & paste text | Time: %8.2f ms\n", get_time_ms(start, end));

    printf("===============================================================\n");

    vfs_clipboard_destroy(clip);
    sb_destroy(sb);
    vfs_destroy(vfs);
}

int main() {
    run_benchmarks(1000);
    return 0;
}
