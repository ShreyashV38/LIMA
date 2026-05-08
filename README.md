# LIMA - Lightweight In-Memory Archive

[![Language](https://img.shields.io/badge/Language-C-00599C.svg?style=flat-square&logo=c)](https://en.cppreference.com/w/c)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](LICENSE)
[![Build](https://img.shields.io/badge/Build-Makefile-blue.svg?style=flat-square)](Makefile)

**LIMA** is a high-performance, POSIX-like Virtual File System (VFS) written in C. It provides an interactive shell for managing a simulated, single-root file hierarchy entirely in memory, featuring a built-in text editor, clipboard management, and multi-level undo/redo support.

---

## Key Features

-  **Interactive REPL**: A robust command-line interface with path completion and history.
-  **Virtual File System**: Fully simulated directory structure (`/`, `..`, absolute/relative paths).
-  **Built-in Editor**: An integrated text editor using a **Gap Buffer** for efficient $O(1)$ insertions and deletions.
-  **System Clipboard**: Support for `copy`, `cut`, and `paste` operations across the VFS.
-  **History Management**: Browser-style navigation (`back`, `forward`) and session-wide `undo`/`redo`.
-  **Persistence**: Save the entire VFS state to a binary file on the host OS and reload it seamlessly.
-  **High Performance**: Optimized data structures including N-ary trees for directory traversal and Gap Buffers for text processing.

---

##  Architecture

LIMA is built on a modular architecture designed for extensibility and efficiency:

###  Core Modules
- **VFS Engine**: Manages the directory tree and file node lifecycle.
- **VFS Session**: Tracks navigation history, undo/redo stacks, and current state.
- **Editor**: A terminal-based UI for real-time text manipulation.
- **Persistence**: Handles serialization and deserialization of the in-memory tree.

###  Data Structures
| Structure | Usage | Efficiency |
| :--- | :--- | :--- |
| **N-ary Tree** | Directory hierarchy & child management | $O(N)$ traversal, $O(1)$ insertion |
| **Gap Buffer** | Core storage for file content | $O(1)$ cursor-local edits |
| **Stack** | Undo/Redo history and navigation tracking | $O(1)$ push/pop |
| **Hash Map** | Internal metadata and fast lookups | $O(1)$ average case |

---

##  Getting Started

### Prerequisites
- **GCC** (GNU Compiler Collection)
- **Make** build utility

### Build & Run
To compile the project and start the LIMA shell:

```bash
# Clone the repository
git clone https://github.com/your-username/LIMA.git
cd LIMA

# Build the project
make all

# Launch the shell
./build/bin/lima.exe
```

### Running Tests
LIMA comes with an extensive suite of unit tests and smoke tests:

```bash
# Run all tests
make test
```

---

##  Command Reference

| Command | Description | Example |
| :--- | :--- | :--- |
| `ls` | List directory contents | `ls` |
| `cd <path>` | Change directory | `cd docs/work` |
| `pwd` | Print working directory | `pwd` |
| `mkdir <dir>` | Create a new directory | `mkdir assets` |
| `touch <file>` | Create an empty file | `touch notes.txt` |
| `edit <file>` | Open the built-in editor | `edit notes.txt` |
| `rm <path>` | Remove a file or directory | `rm old_dir` |
| `copy/cut` | Copy/Cut to clipboard | `copy config.json` |
| `paste` | Paste from clipboard | `paste` |
| `undo/redo` | Revert or repeat actions | `undo` |
| `save/load` | Persist VFS to host disk | `save backup.lima` |

---

## Benchmarking
The project includes specialized benchmarking tools to measure VFS performance under heavy load (deep trees, large files, and high-frequency operations).

```bash
# Run benchmarks
./build/bin/benchmark.exe
```

---



##  License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
