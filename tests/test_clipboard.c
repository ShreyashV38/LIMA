#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/vfs/vfs.h"
#include "../include/core/clipboard.h" 
#include "../include/data_structures/gap_buffer.h"

// gcc -Wall -g tests/test_clipboard.c src/core/clipboard.c src/vfs/*.c src/data_structures/*.c -o test_clipboard

void test_clipboard_deep_copy_independence() {
    // 1. Setup VFS and Clipboard
    Vfs *vfs = vfs_create();
    VfsClipboard *cb = vfs_clipboard_create();
    assert(vfs != NULL);
    assert(cb != NULL);

    // 2. Build a test tree: /folderA/file.txt
    assert(vfs_mkdir(vfs, "folderA") == true);
    assert(vfs_cd(vfs, "folderA") == true);
    assert(vfs_touch(vfs, "file.txt") == true);

    // Write initial text to the file
    GapBuffer *gb_original = vfs_open_file(vfs, "file.txt");
    assert(gb_original != NULL);
    gb_insert(gb_original, "INITIAL_TEXT", 12);

    // Navigate back to root
    assert(vfs_cd(vfs, "/") == true);

    // 3. COPY "folderA" to clipboard O(N)
    assert(vfs_clipboard_copy(cb, vfs, "folderA") == true);
    
    // 4. Modify the original file AFTER copying!
    assert(vfs_cd(vfs, "folderA") == true);
    gb_insert(gb_original, "_MODIFIED", 9); // Text is now "INITIAL_TEXT_MODIFIED"
    assert(vfs_cd(vfs, "/") == true);

    // 5. PASTE the clipboard as a new branch
    assert(vfs_mkdir(vfs, "folderB") == true);
    assert(vfs_cd(vfs, "folderB") == true);
    assert(vfs_clipboard_paste(cb, vfs) == true); // Pastes "folderA" inside "folderB"

    // 6. Verify Independence
    assert(vfs_cd(vfs, "folderA") == true);
    GapBuffer *gb_pasted = vfs_open_file(vfs, "file.txt");
    assert(gb_pasted != NULL);

    size_t len;
    char *pasted_text = gb_get_text(gb_pasted, &len);
    
    // The pasted text should ONLY be "INITIAL_TEXT", proving memory is not shared!
    assert(strcmp(pasted_text, "INITIAL_TEXT") == 0);
    free(pasted_text);

    // 7. Cleanup
    vfs_clipboard_destroy(cb);
    vfs_destroy(vfs);
    printf("  [PASS] test_clipboard_deep_copy_independence\n");
}

void test_clipboard_cut_o1() {
    Vfs *vfs = vfs_create();
    VfsClipboard *cb = vfs_clipboard_create();

    // Create /target_dir
    assert(vfs_mkdir(vfs, "target_dir") == true);

    // Cut /target_dir
    assert(vfs_clipboard_cut(cb, vfs, "target_dir") == true);

    // Verify it is removed from the root directory
    size_t count;
    char **ls_result = vfs_ls(vfs, &count);
    assert(count == 0); // Root should be empty

    // Paste into a new directory /destination
    assert(vfs_mkdir(vfs, "destination") == true);
    assert(vfs_cd(vfs, "destination") == true);
    assert(vfs_clipboard_paste(cb, vfs) == true);

    // Verify it exists in destination
    ls_result = vfs_ls(vfs, &count);
    assert(count == 1);
    assert(strcmp(ls_result[0], "target_dir") == 0);
    free(ls_result[0]);
    free(ls_result);

    // Verify clipboard is empty after pasting a CUT
    assert(cb->current_op == CLIPBOARD_EMPTY);
    assert(cb->node == NULL);

    vfs_clipboard_destroy(cb);
    vfs_destroy(vfs);
    printf("  [PASS] test_clipboard_cut_o1\n");
}

int main() {
    printf("Running Clipboard Tests...\n\n");
    test_clipboard_deep_copy_independence();
    test_clipboard_cut_o1();
    printf("\nAll Clipboard tests passed successfully!\n");
    return 0;
}