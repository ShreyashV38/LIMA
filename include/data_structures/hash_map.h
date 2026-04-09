#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Hash Map ADT, maps string keys to generic void pointers.
 */

typedef struct HashMap HashMap;

/**
 * @brief Create a new hash map.
 * @param initial_capacity The initial number of buckets (can be 0 for default).
 * @return Pointer to the new Hash Map, or NULL on failure.
 */
HashMap *hashmap_create(size_t initial_capacity);

/**
 * @brief Callback function used to free values.
 */
typedef void (*HashMapValueDeallocator)(void *value);

/**
 * @brief Destroy the hash map.
 * @param map Pointer to the hash map.
 * @param deallocator Optional callback to free the values. Keys are freed automatically if they were copied.
 */
void hashmap_destroy(HashMap *map, HashMapValueDeallocator deallocator);

/**
 * @brief Insert or update a key-value pair in the map.
 * @param map Pointer to the hash map.
 * @param key The string key (the map should make a copy of it or assume ownership based on implementation).
 * @param value The value pointer.
 * @return true on success, false on failure.
 */
bool hashmap_put(HashMap *map, const char *key, void *value);

/**
 * @brief Retrieve a value by its key.
 * @param map Pointer to the hash map.
 * @param key The string key.
 * @return The value pointer, or NULL if not found.
 */
void *hashmap_get(const HashMap *map, const char *key);

/**
 * @brief Remove a key-value pair from the map.
 * @param map Pointer to the hash map.
 * @param key The string key.
 * @return The removed value pointer, or NULL if not found (caller is responsible for freeing it).
 */
void *hashmap_remove(HashMap *map, const char *key);

/**
 * @brief Check if the map contains a key.
 * @param map Pointer to the hash map.
 * @param key The string key.
 * @return true if the key exists, false otherwise.
 */
bool hashmap_contains(const HashMap *map, const char *key);

/**
 * @brief Get the number of elements in the map.
 * @param map Pointer to the hash map.
 * @return The number of elements.
 */
size_t hashmap_size(const HashMap *map);

#endif // HASH_MAP_H
