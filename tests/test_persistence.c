#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/vfs/vfs.h"
#include "../include/core/persistence.h"

int main() {
    Vfs *vfs = vfs_create();
    assert(vfs != NULL);

    // Create a simple directory structure
    assert(vfs_mkdir(vfs, "home") == true);
    assert(vfs_cd(vfs, "home") == true);
    assert(vfs_touch(vfs, "file1.txt") == true);
    
    GapBuffer *gb = vfs_open_file(vfs, "file1.txt");
    assert(gb != NULL);
    assert(gb_insert(gb, "Hello, World!", 13) == true);
    
    assert(vfs_mkdir(vfs, "docs") == true);
    
    // Save it
    const char *test_file = "test_dump.bin";
    PersistenceError err = persistence_save_vfs(vfs, test_file);
    if (err != PERSIST_OK) {
        printf("Failed to save: %s\n", persistence_error_string(err));
    }
    assert(err == PERSIST_OK);

    vfs_destroy(vfs);

    // Check if the file exists and its content headers
    FILE *fp = fopen(test_file, "rb");
    assert(fp != NULL);
    
    char magic[5] = {0};
    assert(fread(magic, 1, 4, fp) == 4);
    assert(strcmp(magic, LIMA_MAGIC) == 0);
    
    uint32_t version;
    assert(fread(&version, sizeof(uint32_t), 1, fp) == 1);
    assert(version == LIMA_PERSIST_VERSION);

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);

    printf("TEST PASSED: Persistence save generated %ld bytes successfully!\n", size);
    
    // Load it back
    PersistenceError load_err = PERSIST_OK;
    Vfs *loaded_vfs = persistence_load_vfs(test_file, &load_err);
    if (load_err != PERSIST_OK) {
        printf("Failed to load: %s\n", persistence_error_string(load_err));
    }
    assert(loaded_vfs != NULL);
    assert(load_err == PERSIST_OK);
    
    // Verify loaded structure correctly parsed to RAM
    assert(vfs_cd(loaded_vfs, "home") == true);
    GapBuffer *loaded_gb = vfs_open_file(loaded_vfs, "file1.txt");
    assert(loaded_gb != NULL);
    
    size_t out_len;
    char *text = gb_get_text(loaded_gb, &out_len);
    assert(out_len == 13);
    assert(strncmp(text, "Hello, World!", 13) == 0);
    free(text);
    
    vfs_destroy(loaded_vfs);

    printf("TEST PASSED: Persistence successfully restored VFS to memory!\n");
    
    // Clean up
    remove(test_file);

    return 0;
}
