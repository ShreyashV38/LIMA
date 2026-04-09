#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/data_structures/stack.h"

void test_stack_basic_operations() {
    Stack *stack = stack_create();
    assert(stack != NULL);
    assert(stack_is_empty(stack));
    assert(stack_size(stack) == 0);

    int a = 1, b = 2, c = 3;

    // Test PUSH
    assert(stack_push(stack, &a) == true);
    assert(stack_size(stack) == 1);
    assert(!stack_is_empty(stack));
    assert(stack_peek(stack) == &a);

    assert(stack_push(stack, &b) == true);
    assert(stack_size(stack) == 2);
    assert(stack_peek(stack) == &b);

    assert(stack_push(stack, &c) == true);
    assert(stack_size(stack) == 3);
    assert(stack_peek(stack) == &c);

    // Test POP
    assert(stack_pop(stack) == &c);
    assert(stack_size(stack) == 2);
    assert(stack_peek(stack) == &b);

    assert(stack_pop(stack) == &b);
    assert(stack_size(stack) == 1);
    assert(stack_peek(stack) == &a);

    assert(stack_pop(stack) == &a);
    assert(stack_size(stack) == 0);
    assert(stack_is_empty(stack));

    // Test POP/PEEK on empty stack
    assert(stack_pop(stack) == NULL);
    assert(stack_peek(stack) == NULL);

    stack_destroy(stack, NULL);
    printf("test_stack_basic_operations passed!\n");
}

void test_stack_deallocator() {
    Stack *dyn_stack = stack_create();
    assert(dyn_stack != NULL);

    int *dyn_a = malloc(sizeof(int)); *dyn_a = 10;
    int *dyn_b = malloc(sizeof(int)); *dyn_b = 20;

    assert(stack_push(dyn_stack, dyn_a) == true);
    assert(stack_push(dyn_stack, dyn_b) == true);

    // The deallocator should free dyn_a and dyn_b when the stack is destroyed
    stack_destroy(dyn_stack, free);
    printf("test_stack_deallocator passed!\n");
}

int main() {
    printf("Running stack tests...\n");
    test_stack_basic_operations();
    test_stack_deallocator();
    printf("All stack tests completed successfully.\n");
    return 0;
}
