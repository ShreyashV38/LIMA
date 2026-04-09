#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/data_structures/hash_map.h"

void test_hashmap_basic() {
    HashMap *map = hashmap_create(10);
    assert(map != NULL);
    assert(hashmap_size(map) == 0);

    int val1 = 100, val2 = 200;

    assert(hashmap_put(map, "key1", &val1));
    assert(hashmap_put(map, "key2", &val2));
    assert(hashmap_size(map) == 2);

    assert(hashmap_contains(map, "key1"));
    assert(hashmap_contains(map, "key2"));
    assert(!hashmap_contains(map, "key3"));

    assert(*(int*)hashmap_get(map, "key1") == 100);
    assert(*(int*)hashmap_get(map, "key2") == 200);

    // Update
    int val1_new = 150;
    assert(hashmap_put(map, "key1", &val1_new));
    assert(*(int*)hashmap_get(map, "key1") == 150);
    assert(hashmap_size(map) == 2);

    // Remove
    assert(hashmap_remove(map, "key1") == &val1_new);
    assert(hashmap_size(map) == 1);
    assert(!hashmap_contains(map, "key1"));

    hashmap_destroy(map, NULL);
    printf("test_hashmap_basic passed!\n");
}

void test_hashmap_deallocator() {
    HashMap *map = hashmap_create(5);
    int *dynamic_val = malloc(sizeof(int));
    *dynamic_val = 500;
    
    hashmap_put(map, "dyn", dynamic_val);
    
    // This should free the dynamic_val
    hashmap_destroy(map, free);
    printf("test_hashmap_deallocator passed!\n");
}

int main() {
    printf("Running hash map tests...\n");
    test_hashmap_basic();
    test_hashmap_deallocator();
    printf("All hash map tests passed!\n");
    return 0;
}
