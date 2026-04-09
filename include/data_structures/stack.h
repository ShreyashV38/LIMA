#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Generic Stack ADT.
 */

typedef struct Stack Stack;

/**
 * @brief Create a new stack.
 * @return A new stack instance, or NULL on failure.
 */
Stack *stack_create(void);

/**
 * @brief Callback function used to free stack items.
 */
typedef void (*StackItemDeallocator)(void *item);

/**
 * @brief Destroy the stack and free underlying memory.
 * @param stack Pointer to the stack.
 * @param deallocator Optional callback to free the node data.
 */
void stack_destroy(Stack *stack, StackItemDeallocator deallocator);

/**
 * @brief Push an item onto the stack.
 * @param stack Pointer to the stack.
 * @param item Pointer to the item to push.
 * @return true on success, false on failure.
 */
bool stack_push(Stack *stack, void *item);

/**
 * @brief Pop an item from the stack.
 * @param stack Pointer to the stack.
 * @return Pointer to the popped item, or NULL if the stack is empty.
 */
void *stack_pop(Stack *stack);

/**
 * @brief Peek at the top item of the stack without removing it.
 * @param stack Pointer to the stack.
 * @return Pointer to the top item, or NULL if the stack is empty.
 */
void *stack_peek(const Stack *stack);

/**
 * @brief Check if the stack is empty.
 * @param stack Pointer to the stack.
 * @return true if empty, false otherwise.
 */
bool stack_is_empty(const Stack *stack);

/**
 * @brief Get the number of items in the stack.
 * @param stack Pointer to the stack.
 * @return The size of the stack.
 */
size_t stack_size(const Stack *stack);

#endif // STACK_H
