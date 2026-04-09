#include "../../include/data_structures/stack.h"
#include <stdlib.h>

struct Stack {
    void **items;
    size_t capacity;
    size_t count;
};

Stack *stack_create(void) {
    Stack *s = malloc(sizeof(Stack));
    if (!s) return NULL;
    
    s->capacity = 16;
    s->count = 0;
    s->items = malloc(sizeof(void *) * s->capacity);
    if (!s->items) {
        free(s);
        return NULL;
    }
    return s;
}

void stack_destroy(Stack *stack, StackItemDeallocator deallocator) {
    if (!stack) return;
    
    if (deallocator) {
        for (size_t i = 0; i < stack->count; i++) {
            if (stack->items[i]) {
                deallocator(stack->items[i]);
            }
        }
    }
    
    free(stack->items);
    free(stack);
}

bool stack_push(Stack *stack, void *item) {
    if (!stack) return false;
    
    if (stack->count >= stack->capacity) {
        size_t new_cap = stack->capacity * 2;
        void **new_items = realloc(stack->items, sizeof(void *) * new_cap);
        if (!new_items) return false;
        stack->items = new_items;
        stack->capacity = new_cap;
    }
    
    stack->items[stack->count++] = item;
    return true;
}

void *stack_pop(Stack *stack) {
    if (!stack || stack->count == 0) return NULL;
    return stack->items[--stack->count];
}

void *stack_peek(const Stack *stack) {
    if (!stack || stack->count == 0) return NULL;
    return stack->items[stack->count - 1];
}

bool stack_is_empty(const Stack *stack) {
    return !stack || stack->count == 0;
}

size_t stack_size(const Stack *stack) {
    return stack ? stack->count : 0;
}
