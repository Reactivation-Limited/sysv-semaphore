# sysv-semaphore

Node.js bindings for Unix System V Semaphores.

Should work on any system that supports System V semaphores. YMMV. If it breaks, let me know, or better still fix it and make a PR.

## Requirements

- Node.js 14.x or later
- A Unix-like operating system with System V IPC support
- Prebuilt binaries supported: Linux (x64, arm64) OSX (x64, arm64)
- otherwise, C++ build tools (for native compilation)

## Installation

```bash
npm install sysv-semaphore
```

For other platforms, the module will be built from source during installation.

## Usage

A simple API that uses the Unix System V `semget` family of system calls to create and use semaphores.

All operations are performed with `SEM_UNDO` ensuring that semaphores are released by the kernel when the process exits.

### API Reference

#### Creating/Opening Semaphores

```javascript
// Create a semaphore, but only if one does not already exist
const sem = Semaphore.createExclusive('/path/to/some/file', 10);

// Open an existing semaphore, or create it if it doesn't exist
const sem = Semaphore.create('/path/to/some/file', 10);

// Open an existing semaphore
const sem = Semaphore.open('/path/to/some/file');
```

#### Semaphore Operations

```javascript
// Block waiting for the semaphore (not recommended in most cases)
sem.wait();

// Non-blocking attempt to acquire semaphore
if (sem.trywait()) {
  // Critical section
}

// Get current value
const value = sem.valueOf();

// Release one unit
sem.post();

// Aquire multiple units
if (sem.trywait(10)) {
  // Critical section
}

// Release multiple units
sem.post(10);

// Clean up
sem.close();
```

### Basic Example

```javascript
// myModule.js
const { Semaphore } = require('sysv-semaphore');

// Create a module-scoped semaphore
// Initial value of 1 makes this a mutex/binary semaphore
const mutex = Semaphore.create('/path/to/some/file', 1);

// Example function that uses the semaphore
async function doSomethingExclusive() {
  if (mutex.trywait()) {
    try {
      // Critical section
      console.log('Acquired semaphore');
      await someAsyncWork();
    } finally {
      // Release the semaphore
      mutex.post();
    }
  } else {
    console.log('Semaphore was not available');
  }
}

// Another example with a counting semaphore
const poolSemaphore = Semaphore.create('/path/to/some/file', 5); // Allow 5 concurrent operations

async function useResourcePool() {
  // Aquire 2 resources
  if (poolSemaphore.trywait(2)) {
    try {
      // Use one of the 5 available resources
      await doSomeWork();
    } finally {
      // Release 2 resources
      poolSemaphore.post(2);
    }
  } else {
    // Pool is full, handle accordingly
  }
}

// Semaphores will be automatically cleaned up when the process exits
```

## Best Practices

1. Always use `try/finally` blocks to ensure semaphores are properly closed
2. Prefer `trywait()` over `wait()` to avoid blocking the Node.js event loop
3. Use `create()` instead of `createExclusive()` for better crash recovery
4. Keep critical sections as short as possible
5. Handle errors appropriately - check `error.code` for specific error conditions

## Error Handling

All operations can throw system errors. The error object will have:

- `error.code`: Standard Node.js error codes (e.g., 'EACCES', 'EINVAL')
- `error.message`: Includes the failing system call name

Common error codes:

- `EACCES`: Permission denied
- `EEXIST`: Semaphore exists (with createExclusive)
- `ENOENT`: Semaphore does not exist
- `EINVAL`: Invalid argument

## Troubleshooting

1. **Semaphore not being released**:

   - Ensure `close()` is called in a `finally` block
   - Check if process crashed while holding semaphore

2. **Permission errors**:

   - Check file permissions of the key file
   - Verify user has appropriate system permissions

3. **Resource limits**:
   - Check system semaphore limits (`ipcs -l`)
   - Clean up unused semaphores (`ipcs -s` to list, `ipcrm` to remove)

## Notes on Garbage Collection

While the Node.js garbage collector will eventually clean up semaphores, it's best practice to explicitly call `close()` when done. This ensures:

- Timely resource release
- Predictable cleanup
- Better system resource management

## License

This is free software with no warranty expressed or implied. If it breaks you get to keep both pieces. See the `LICENSE`.
