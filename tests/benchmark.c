#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/vfs/vfs.h"
#include "../include/vfs/vfs_session.h"
#include "../include/core/clipboard.h"
#include "../include/core/persistence.h"
#include "../include/data_structures/gap_buffer.h"

// Helper to calculate time in milliseconds
static double get_time_ms(clock_t start, clock_t end) {
    return ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
}

static void benchmark_basic_operations() {
    Vfs *vfs = vfs_create();
    clock_t start, end;
    
    int num_ops = 10000;
    
    // Benchmark mkdir
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        char dirname[64];
        snprintf(dirname, sizeof(dirname), "dir_%d", i);
        vfs_mkdir(vfs, dirname);
    }
    end = clock();
    printf("Operation: mkdir       | Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    // Benchmark cd & pwd
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        vfs_cd(vfs, "/");
        char dirname[64];
        snprintf(dirname, sizeof(dirname), "dir_%d", i);
        vfs_cd(vfs, dirname);
        char *pwd = vfs_pwd(vfs);
        free(pwd);
    }
    end = clock();
    printf("Operation: cd & pwd    | Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    // Benchmark touch
    vfs_cd(vfs, "/");
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "file_%d.txt", i);
        vfs_touch(vfs, filename);
    }
    end = clock();
    printf("Operation: touch       | Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    // Benchmark file open and write (gap buffer)
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "file_%d.txt", i);
        GapBuffer *gb = vfs_open_file(vfs, filename);
        if (gb) {
            gb_insert(gb, "Hello, LIMA benchmark!", 22);
        }
    }
    end = clock();
    printf("Operation: file write  | Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    // Benchmark rm (delete both directories and files)
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        char dirname[64];
        snprintf(dirname, sizeof(dirname), "dir_%d", i);
        vfs_rm(vfs, dirname); 
    }
    for (int i = 0; i < num_ops; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "file_%d.txt", i);
        vfs_rm(vfs, filename);
    }
    end = clock();
    printf("Operation: rm (delete) | Iterations: %-6d | Time: %8.2f ms\n", num_ops * 2, get_time_ms(start, end));

    vfs_destroy(vfs);
}

static void benchmark_clipboard() {
    Vfs *vfs = vfs_create();
    VfsClipboard *clip = vfs_clipboard_create();
    clock_t start, end;
    
    int num_ops = 5000;
    
    for (int i = 0; i < num_ops; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "clipfile_%d", i);
        vfs_touch(vfs, filename);
    }
    
    // Benchmark copy
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "clipfile_%d", i);
        vfs_clipboard_copy(clip, vfs, filename);
    }
    end = clock();
    printf("Operation: clip copy   | Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    vfs_mkdir(vfs, "paste_target");
    vfs_cd(vfs, "paste_target");
    
    // Copy just one file for rapid duplicate pasting
    vfs_cd(vfs, "/");
    vfs_clipboard_copy(clip, vfs, "clipfile_0");
    vfs_cd(vfs, "paste_target");
    
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        vfs_clipboard_paste(clip, vfs);
    }
    end = clock();
    printf("Operation: clip paste  | Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    vfs_clipboard_destroy(clip);
    vfs_destroy(vfs);
}

static void benchmark_session_history() {
    VfsSession *sess = vfs_session_create();
    clock_t start, end;
    
    int num_ops = 5000; 
    
    start = clock();
    for (int i = 0; i < num_ops; i++) {
        char dirname[64];
        snprintf(dirname, sizeof(dirname), "sdir_%d", i);
        vfs_session_mkdir(sess, dirname);
        vfs_session_cd(sess, dirname);
    }
    end = clock();
    printf("Operation: session cd  | Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));

    start = clock();
    for (int i = 0; i < num_ops; i++) {
        vfs_session_back(sess);
    }
    end = clock();
    printf("Operation: session back| Iterations: %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    vfs_session_destroy(sess);
}

static void benchmark_persistence() {
    Vfs *vfs = vfs_create();
    int num_ops = 5000;
    
    for (int i = 0; i < num_ops; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "pers_file_%d.txt", i);
        vfs_touch(vfs, filename);
        GapBuffer *gb = vfs_open_file(vfs, filename);
        if (gb) gb_insert(gb, "Persistence Test Data", 21);
    }
    
    clock_t start, end;
    start = clock();
    persistence_save_vfs(vfs, "benchmark_save.lima");
    end = clock();
    printf("Operation: persist save| Nodes:      %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    start = clock();
    PersistenceError err;
    Vfs *loaded = persistence_load_vfs("benchmark_save.lima", &err);
    end = clock();
    printf("Operation: persist load| Nodes:      %-6d | Time: %8.2f ms\n", num_ops, get_time_ms(start, end));
    
    remove("benchmark_save.lima");
    vfs_destroy(vfs);
    if (loaded) vfs_destroy(loaded);
}

int main() {
    printf("===============================================================\n");
    printf("             LIMA Virtual File System - Benchmark\n");
    printf("===============================================================\n");
    benchmark_basic_operations();
    benchmark_clipboard();
    benchmark_session_history();
    benchmark_persistence();
    printf("===============================================================\n");
    return 0;
}
