# sysv-semaphore

node bindings for Unix System V Semaphores.

## Why use this lib?

Get all the goodness and resiliece of System V semaphores in your node apps.

Unlike posix semaphores, operations on System V semaphores are automatically undoe when a process exits, even if the process crashed thanks to `SEM_UNDO`. See usage below.

This library maintains a reference count using a second semaphore so when the last process closes the semaphore it will be removed from the system. No dangling semaphores, mostly.

Edge case: if the last process that has the semaphore open crashes, the semaphore won't be removed.

Built and tested on OSX. Also built and tested on Debian Linux.

Should work on any system that support Sys V semaphores. YMMV. If it breaks, let me know, or better still fix it and make a PR.

## General information and philosophical ramblings

There is no hand-holding here - these are thin wrappers over the system calls. If a call throws a system error, treat is as a warning that you've made a mistake in your implementation you should investigate. Failed calls will throw an error with `error.code` set to a string matching the errno, just like `fs` does it, and also tell yu which underlying system call the failure came from.

This is free software with no warranty expressed or implied. If it breaks you get to keep both pieces. See the `LICENSE`.

## Installation

`npm install sysv-semaphore`

## Usage

A simple API that uses the Unix SystemV `semget` family of system calls to create and use semaphores.

All operations are performed with `SEM_UNDO` ensuring that semaphores are released by the kernel when the process exits.

For error codes see `man -s2 semget`, `man -s2 semctl` and `man -s2 semop`.

In the case of `EINTR`, the operation will be automatically retried.

See [https://stackoverflow.com/questions/368322/differences-between-system-v-and-posix-semaphores] for a discussion of the differences between POSIX and SYSV semaphores.

TL;DR; POSIX semaphores are lightweight, but have failure modes when a process crashes. SystemV semaphores are "heavier", but more much more robust.

Blocking and non-blocking operations are both possible. Typically you will want to use the non-blocking version. See https://nodejs.org/en/learn/asynchronous-work/overview-of-blocking-vs-non-blocking for a explanation of how blocking calls work in node.

Most of the time you will want to `create`, then use `trywait` to aquire a lock and `post` to release it. Use `op` when you want to release aquired locks en-masse.

```javascript
const { Semaphore } = require('OSiX');

// create a semaphore, but only if one does not already exist, and set the initial value to 10
const sem = Semaphore.createExclusive('/path/to/some/file', 10);

// open an existing semaphore, or, if it does not exist then create it and set the inital value to 10
// Note: if the semaphore exists, then the initial value is IGNORED
// (most of the time, this is the function you'll want to use)
const sem = Semaphore.create('/path/to/some/file', 10);

// open an existing semaphore
const sem = Semaphore.open('/path/to/some/file');

// block waiting for the semaphore to be greater than 0
sem.wait();

// if the semaphore value is greater than 0, decrement it and return true, otherwise return false
if (sem.trywait()) {
  // enter crital section of code
} else {
  // do something else
}

sem.valueOf(); // get the current value of the semaphore

// release a semaphore you aquired using wait or trywait. Increments the semamphore value.
// never blocks.
// Note: this can increase the semaphore to a value greater than the initial value!
sem.post();

// release many semaphores you aquired using wait or trywait. Adds the argument to the semamphore value.
// never blocks.
// Note: this can increase the semaphore to a value greater than the initial value!
sem.post(10);

sem.close(); // decrements the semaphore reference count and unlinks the semphore if this was the last reference by ANY process
```

The `create`, `createExclusive` and `open` calls use `semget` and `semctl` under the hood.

## Caveats

If the last process with the semaphore open aborts, then the semaphore will not be unlinked, although it's value will be corrected due to the `SEM_UNDO` semantics. Consequently, it is most robust to use `create` rather than `createExclusive` as the semaphore will already exist if this happens.
