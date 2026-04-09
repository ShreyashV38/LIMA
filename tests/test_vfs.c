#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/vfs/vfs.h"

static void free_ls(char **names, size_t n) {
    if (!names) return;
    for (size_t i = 0; i < n; i++) free(names[i]);
    free(names);
}

void test_vfs_basic_mkdir_cd_pwd_ls_open() {
    Vfs *vfs = vfs_create();
    assert(vfs != NULL);

    char *pwd = vfs_pwd(vfs);
    assert(pwd != NULL);
    assert(strcmp(pwd, "/") == 0);
    free(pwd);

    assert(vfs_mkdir(vfs, "home") == true);
    assert(vfs_mkdir(vfs, "tmp") == true);
    assert(vfs_mkdir(vfs, "home") == false); // duplicate

    size_t n = 0;
    char **names = vfs_ls(vfs, &n);
    assert(n == 2);
    assert(names != NULL);
    // order is insertion order in nary_tree append
    assert(strcmp(names[0], "home") == 0);
    assert(strcmp(names[1], "tmp") == 0);
    free_ls(names, n);

    assert(vfs_cd(vfs, "home") == true);
    pwd = vfs_pwd(vfs);
    assert(strcmp(pwd, "/home") == 0);
    free(pwd);

    assert(vfs_mkdir(vfs, "docs") == true);
    assert(vfs_cd(vfs, "docs") == true);
    pwd = vfs_pwd(vfs);
    assert(strcmp(pwd, "/home/docs") == 0);
    free(pwd);

    assert(vfs_cd(vfs, "..") == true);
    assert(vfs_cd(vfs, "/tmp") == true);
    pwd = vfs_pwd(vfs);
    assert(strstr(pwd, "tmp") != NULL);
    free(pwd);

    assert(vfs_touch(vfs, "note.txt") == true);
    GapBuffer *gb = vfs_open_file(vfs, "note.txt");
    assert(gb != NULL);
    assert(gb_insert(gb, "Hello", 5) == true);
    gb_delete_before(gb, 2);
    char *text = gb_get_text(gb, NULL);
    assert(strcmp(text, "Hel") == 0);
    free(text);

    vfs_destroy(vfs);
    printf("test_vfs_basic_mkdir_cd_pwd_ls_open passed!\n");
}

int main() {
    printf("Running VFS tests...\n");
    test_vfs_basic_mkdir_cd_pwd_ls_open();
    printf("All VFS tests passed!\n");
    return 0;
}
