#include "../../include/data_structures/hash_map.h"
#include <stdlib.h>
#include <string.h>

typedef struct HashMapEntry {
    char *key;
    void *value;
    struct HashMapEntry *next;
} HashMapEntry;

struct HashMap {
    HashMapEntry **buckets;
    size_t capacity;
    size_t size;
};

// djb2 hash function
static unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

static char *internal_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p) memcpy(p, s, len);
    return p;
}

HashMap *hashmap_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 32;
    
    HashMap *map = malloc(sizeof(HashMap));
    if (!map) return NULL;
    
    map->buckets = calloc(initial_capacity, sizeof(HashMapEntry*));
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    map->capacity = initial_capacity;
    map->size = 0;
    
    return map;
}

void hashmap_destroy(HashMap *map, HashMapValueDeallocator deallocator) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        HashMapEntry *current = map->buckets[i];
        while (current) {
            HashMapEntry *next = current->next;
            free(current->key);
            if (deallocator && current->value) {
                deallocator(current->value);
            }
            free(current);
            current = next;
        }
    }
    
    free(map->buckets);
    free(map);
}

bool hashmap_put(HashMap *map, const char *key, void *value) {
    if (!map || !key) return false;
    
    unsigned long hash = djb2_hash(key);
    size_t index = hash % map->capacity;
    
    HashMapEntry *current = map->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;
            return true;
        }
        current = current->next;
    }
    
    HashMapEntry *new_entry = malloc(sizeof(HashMapEntry));
    if (!new_entry) return false;
    
    new_entry->key = internal_strdup(key);
    if (!new_entry->key) {
        free(new_entry);
        return false;
    }
    
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
    
    return true;
}

void *hashmap_get(const HashMap *map, const char *key) {
    if (!map || !key) return NULL;
    
    unsigned long hash = djb2_hash(key);
    size_t index = hash % map->capacity;
    
    HashMapEntry *current = map->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    
    return NULL;
}

void *hashmap_remove(HashMap *map, const char *key) {
    if (!map || !key) return NULL;
    
    unsigned long hash = djb2_hash(key);
    size_t index = hash % map->capacity;
    
    HashMapEntry *current = map->buckets[index];
    HashMapEntry *prev = NULL;
    
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->buckets[index] = current->next;
            }
            
            void *val = current->value;
            free(current->key);
            free(current);
            map->size--;
            return val;
        }
        prev = current;
        current = current->next;
    }
    
    return NULL;
}

bool hashmap_contains(const HashMap *map, const char *key) {
    if (!map || !key) return false;
    unsigned long hash = djb2_hash(key);
    size_t index = hash % map->capacity;
    
    HashMapEntry *current = map->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

size_t hashmap_size(const HashMap *map) {
    return map ? map->size : 0;
}
