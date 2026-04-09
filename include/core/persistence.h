#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <stdbool.h>
#include <stdint.h>
#include "../vfs/vfs.h"

#define LIMA_MAGIC "LIMA"
#define LIMA_PERSIST_VERSION 1

typedef enum {
    PERSIST_OK = 0,
    PERSIST_ERR_FILE_NOT_FOUND = 1,
    PERSIST_ERR_CORRUPT_MAGIC = 2,
    PERSIST_ERR_VERSION_MISMATCH = 3,
    PERSIST_ERR_TRUNCATED = 4,
    PERSIST_ERR_ALLOCATION = 5,
    PERSIST_ERR_WRITE_FAILED = 6,
    PERSIST_ERR_VFS_STATE = 7
} PersistenceError;

/**
 * @brief Get human-readable string for an error code.
 */
const char *persistence_error_string(PersistenceError err);

/**
 * @brief Serialize the entire VFS (from root down) to a binary file on the host OS.
 * @param vfs The virtual file system to save.
 * @param filepath The host OS filepath to write to.
 * @return PERSIST_OK on success, or specific PersistenceError on failure.
 */
PersistenceError persistence_save_vfs(const Vfs *vfs, const char *filepath);

/**
 * @brief Parse a binary file and recreate the VFS in memory.
 * @param filepath The host OS filepath to read from.
 * @param out_error Pointer to error code (optional, can be NULL).
 * @return A newly allocated Vfs*, or NULL if the file is invalid/corrupt.
 */
Vfs *persistence_load_vfs(const char *filepath, PersistenceError *out_error);

#endif // PERSISTENCE_H
