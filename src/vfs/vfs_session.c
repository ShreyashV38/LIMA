#include "../../include/vfs/vfs_session.h"

#include <stdlib.h>
#include <string.h>

#include "../../include/data_structures/stack.h"

typedef enum ActionType {
    ACT_MKDIR = 1,
    ACT_TOUCH = 2,
    ACT_CD = 3
} ActionType;

typedef struct Action {
    ActionType type;
    NaryTreeNode *parent_dir; // for mkdir/touch
    char *name;               // for mkdir/touch

    NaryTreeNode *from_dir;   // for cd
    NaryTreeNode *to_dir;     // for cd
} Action;

struct VfsSession {
    Vfs *vfs;
    Stack *undo;
    Stack *redo;
    Stack *back;
    Stack *forward;
};

static char *s_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static void action_free(void *p) {
    if (!p) return;
    Action *a = (Action *)p;
    free(a->name);
    free(a);
}

static void stack_clear(Stack *st, StackItemDeallocator dealloc) {
    if (!st) return;
    while (!stack_is_empty(st)) {
        void *x = stack_pop(st);
        if (dealloc && x) dealloc(x);
    }
}

static bool push_action(Stack *st, Action *a) {
    if (!a) return false;
    if (!stack_push(st, a)) {
        action_free(a);
        return false;
    }
    return true;
}

static Action *action_new_mk(ActionType t, NaryTreeNode *parent, const char *name) {
    Action *a = (Action *)calloc(1, sizeof(Action));
    if (!a) return NULL;
    a->type = t;
    a->parent_dir = parent;
    a->name = s_strdup(name);
    if (!a->name) {
        free(a);
        return NULL;
    }
    return a;
}

static Action *action_new_cd(NaryTreeNode *from, NaryTreeNode *to) {
    Action *a = (Action *)calloc(1, sizeof(Action));
    if (!a) return NULL;
    a->type = ACT_CD;
    a->from_dir = from;
    a->to_dir = to;
    return a;
}

VfsSession *vfs_session_create(void) {
    VfsSession *s = (VfsSession *)calloc(1, sizeof(VfsSession));
    if (!s) return NULL;

    s->vfs = vfs_create();
    s->undo = stack_create();
    s->redo = stack_create();
    s->back = stack_create();
    s->forward = stack_create();

    if (!s->vfs || !s->undo || !s->redo || !s->back || !s->forward) {
        vfs_session_destroy(s);
        return NULL;
    }

    return s;
}

void vfs_session_destroy(VfsSession *s) {
    if (!s) return;
    if (s->undo) stack_destroy(s->undo, action_free);
    if (s->redo) stack_destroy(s->redo, action_free);
    if (s->back) stack_destroy(s->back, NULL);
    if (s->forward) stack_destroy(s->forward, NULL);
    if (s->vfs) vfs_destroy(s->vfs);
    free(s);
}

Vfs *vfs_session_vfs(VfsSession *s) {
    return s ? s->vfs : NULL;
}

static void record_new_action(VfsSession *s, Action *a) {
    // new user action invalidates redo
    stack_clear(s->redo, action_free);
    push_action(s->undo, a);
}

bool vfs_session_mkdir(VfsSession *s, const char *name) {
    if (!s || !s->vfs) return false;
    NaryTreeNode *parent = vfs_cwd(s->vfs);
    if (!vfs_mkdir(s->vfs, name)) return false;

    Action *a = action_new_mk(ACT_MKDIR, parent, name);
    if (!a) return false; // mkdir already happened; keep FS consistent even if history OOM
    record_new_action(s, a);
    return true;
}

bool vfs_session_touch(VfsSession *s, const char *name) {
    if (!s || !s->vfs) return false;
    NaryTreeNode *parent = vfs_cwd(s->vfs);
    if (!vfs_touch(s->vfs, name)) return false;

    Action *a = action_new_mk(ACT_TOUCH, parent, name);
    if (!a) return false;
    record_new_action(s, a);
    return true;
}

bool vfs_session_cd(VfsSession *s, const char *path) {
    if (!s || !s->vfs) return false;
    NaryTreeNode *from = vfs_cwd(s->vfs);
    if (!vfs_cd(s->vfs, path)) return false;
    NaryTreeNode *to = vfs_cwd(s->vfs);

    // navigation history
    if (from && to && from != to) {
        stack_push(s->back, from);
        stack_clear(s->forward, NULL);
    }

    Action *a = action_new_cd(from, to);
    if (!a) return true;
    record_new_action(s, a);
    return true;
}

bool vfs_session_back(VfsSession *s) {
    if (!s || !s->vfs || !s->back || stack_is_empty(s->back)) return false;
    NaryTreeNode *cur = vfs_cwd(s->vfs);
    NaryTreeNode *prev = (NaryTreeNode *)stack_pop(s->back);
    if (!prev) return false;
    if (cur) stack_push(s->forward, cur);
    return vfs_chdir_node(s->vfs, prev);
}

bool vfs_session_forward(VfsSession *s) {
    if (!s || !s->vfs || !s->forward || stack_is_empty(s->forward)) return false;
    NaryTreeNode *cur = vfs_cwd(s->vfs);
    NaryTreeNode *next = (NaryTreeNode *)stack_pop(s->forward);
    if (!next) return false;
    if (cur) stack_push(s->back, cur);
    return vfs_chdir_node(s->vfs, next);
}

static bool undo_apply(VfsSession *s, Action *a) {
    if (!s || !a) return false;
    switch (a->type) {
        case ACT_MKDIR:
        case ACT_TOUCH:
            return vfs_rm_at(s->vfs, a->parent_dir, a->name);
        case ACT_CD:
            return vfs_chdir_node(s->vfs, a->from_dir);
        default:
            return false;
    }
}

static bool redo_apply(VfsSession *s, Action *a) {
    if (!s || !a) return false;
    switch (a->type) {
        case ACT_MKDIR:
            return vfs_mkdir_at(s->vfs, a->parent_dir, a->name);
        case ACT_TOUCH:
            return vfs_touch_at(s->vfs, a->parent_dir, a->name);
        case ACT_CD:
            return vfs_chdir_node(s->vfs, a->to_dir);
        default:
            return false;
    }
}

bool vfs_session_undo(VfsSession *s) {
    if (!s || !s->undo || stack_is_empty(s->undo)) return false;
    Action *a = (Action *)stack_pop(s->undo);
    if (!a) return false;
    if (!undo_apply(s, a)) {
        // couldn't apply; drop the action to avoid loops
        action_free(a);
        return false;
    }
    return push_action(s->redo, a);
}

bool vfs_session_redo(VfsSession *s) {
    if (!s || !s->redo || stack_is_empty(s->redo)) return false;
    Action *a = (Action *)stack_pop(s->redo);
    if (!a) return false;
    if (!redo_apply(s, a)) {
        action_free(a);
        return false;
    }
    return push_action(s->undo, a);
}

