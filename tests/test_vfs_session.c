#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../include/vfs/vfs_session.h"

static char *pwd(VfsSession *s) {
    return vfs_pwd(vfs_session_vfs(s));
}

void test_session_undo_redo_mkdir_touch_cd() {
    VfsSession *s = vfs_session_create();
    assert(s != NULL);

    assert(vfs_session_mkdir(s, "a") == true);
    assert(vfs_session_touch(s, "f.txt") == true);
    assert(vfs_session_cd(s, "a") == true);

    char *p = pwd(s);
    assert(strcmp(p, "/a") == 0);
    free(p);

    // undo cd -> back to /
    assert(vfs_session_undo(s) == true);
    p = pwd(s);
    assert(strcmp(p, "/") == 0);
    free(p);

    // undo touch -> f.txt removed
    assert(vfs_session_undo(s) == true);
    size_t n = 0;
    char **names = vfs_ls(vfs_session_vfs(s), &n);
    assert(n == 1);
    assert(strcmp(names[0], "a") == 0);
    free(names[0]);
    free(names);

    // undo mkdir -> a removed
    assert(vfs_session_undo(s) == true);
    n = 0;
    names = vfs_ls(vfs_session_vfs(s), &n);
    assert(n == 0);
    assert(names == NULL);

    // redo mkdir, touch, cd
    assert(vfs_session_redo(s) == true);
    assert(vfs_session_redo(s) == true);
    assert(vfs_session_redo(s) == true);
    p = pwd(s);
    assert(strcmp(p, "/a") == 0);
    free(p);

    vfs_session_destroy(s);
    printf("test_session_undo_redo_mkdir_touch_cd passed!\n");
}

void test_session_back_forward() {
    VfsSession *s = vfs_session_create();
    assert(s != NULL);

    assert(vfs_session_mkdir(s, "a") == true);
    assert(vfs_session_mkdir(s, "b") == true);

    assert(vfs_session_cd(s, "a") == true);
    assert(vfs_session_cd(s, "..") == true);
    assert(vfs_session_cd(s, "b") == true);

    char *p = pwd(s);
    assert(strcmp(p, "/b") == 0);
    free(p);

    assert(vfs_session_back(s) == true);
    p = pwd(s);
    assert(strcmp(p, "/") == 0);
    free(p);

    assert(vfs_session_back(s) == true);
    p = pwd(s);
    assert(strcmp(p, "/a") == 0);
    free(p);

    assert(vfs_session_forward(s) == true);
    p = pwd(s);
    assert(strcmp(p, "/") == 0);
    free(p);

    assert(vfs_session_forward(s) == true);
    p = pwd(s);
    assert(strcmp(p, "/b") == 0);
    free(p);

    vfs_session_destroy(s);
    printf("test_session_back_forward passed!\n");
}

int main() {
    printf("Running VFS session tests...\n");
    test_session_undo_redo_mkdir_touch_cd();
    test_session_back_forward();
    printf("All VFS session tests passed!\n");
    return 0;
}

