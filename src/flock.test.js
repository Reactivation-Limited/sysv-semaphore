const { Flock } = require('../build/Release/OSX.node');
const { mkdir, open, unlink } = require('node:fs/promises');
const { fork } = require('node:child_process');
const debug = require('debug')('flock-test');

describe('Semaphore', () => {
  it('should be defined', () => {
    expect(Flock).not.toBe(undefined);
  });

  it('should not lock non-files', async () => {
    expect(() => Flock.share(-1)).toThrow('EBADF');
    expect(() => Flock.share(0)).toThrow('ENOTSUP');
  });

  describe('locking', () => {
    const FILE = './tmp/test-flock-' + process.pid;
    let F;
    beforeAll(async () => {
      F = await open(FILE, 'wx+');
    });
    afterAll(async () => {
      await F.close();
      await unlink(FILE);
    });

    it('should lock files', async () => {
      expect(() => Flock.share(F.fd)).not.toThrow();
      expect(() => Flock.exclusive(F.fd)).not.toThrow();
      expect(() => Flock.unlock(F.fd)).not.toThrow();
      expect(() => Flock.exclusive(F.fd)).not.toThrow();
      expect(() => Flock.share(F.fd)).not.toThrow();
      expect(() => Flock.unlock(F.fd)).not.toThrow();
    });
  });

  describe('process cooperation', () => {
    let child;
    let childMessages;
    let F;
    let file;
    beforeAll(async () => {
      try {
        await mkdir('tmp');
      } catch (error) {
        if (error.code !== 'EEXIST') {
          throw error;
        }
      }

      child = fork('./test/flock-child.js', ['child'], {
        stdio: [process.stdin, process.stdout, process.stderr, 'ipc'],
        env: { DEBUG_COLORS: '', DEBUG: process.env.DEBUG }
      });

      process.on('SIGINT', () => {
        child.kill('SIGINT');
      });

      async function* generator(child) {
        let resolve;
        let reject;
        let waiting;

        debug('g begin');

        try {
          child.on('message', (message) => {
            try {
              if (waiting) {
                debug('g rx', message);
                resolve(message);
              } else {
                throw 'unexpected message' + JSON.stringify(message);
              }
            } catch (whatever) {
              debug('g throw');
              if (waiting) {
                reject(whatever);
              }
              throw whatever;
            } finally {
              waiting = null;
            }
          });

          for (;;) {
            waiting = new Promise((res, rej) => ((resolve = res), (reject = rej)));
            debug('g yield');
            const command = yield waiting;
            debug('g command', command);
            if (command) {
              child.send(command);
            }
          }
        } finally {
          debug('g finally');
          child.kill();
        }
      }
      childMessages = generator(child);
      const n = await childMessages.next();
      debug(n);
      file = n.value;
    });
    beforeEach(async () => {
      F = await open(file, 'r');
    });
    afterEach(async () => {
      await F.close();
    });
    afterAll(() => {
      childMessages.return();
      child.kill();
    });

    describe('blocking calls', () => {
      describe('exclusive', () => {
        it('should not block and get an exclusive lock when another process has no lock', async () => {
          const start = performance.now();

          expect(() => Flock.exclusive(F.fd)).not.toThrow();
          expect(performance.now() - start).toBeLessThan(100);
        });

        it('should block and get an exclusive lock when another process releases a shared lock', async () => {
          const start = performance.now();
          const share = childMessages.next('share');

          await expect(share).resolves.toEqual({
            value: 'share',
            done: false
          });

          const unlocked = childMessages.next('unlock-later');

          expect(() => Flock.exclusive(F.fd)).not.toThrow();
          expect(performance.now() - start).toBeGreaterThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should block and get an exclusive lock when another process releases an exclusive lock', async () => {
          const start = performance.now();
          const share = childMessages.next('exclusive');

          await expect(share).resolves.toEqual({
            value: 'exclusive',
            done: false
          });

          const unlocked = childMessages.next('unlock-later');

          expect(() => Flock.exclusive(F.fd)).not.toThrow();
          expect(performance.now() - start).toBeGreaterThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });
      });

      describe('shared', () => {
        it('should not block and get a shared lock when another process has an shared lock', async () => {
          const start = performance.now();
          const shared = childMessages.next('share');

          await expect(shared).resolves.toEqual({
            value: 'share',
            done: false
          });

          const unlocked = childMessages.next('unlock-later');

          expect(() => Flock.share(F.fd)).not.toThrow();
          expect(performance.now() - start).toBeLessThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should block and get a shared lock when another process has an exclusive lock', async () => {
          const start = performance.now();
          const share = childMessages.next('exclusive');

          await expect(share).resolves.toEqual({
            value: 'exclusive',
            done: false
          });

          const unlocked = childMessages.next('unlock-later');

          expect(() => Flock.share(F.fd)).not.toThrow();
          expect(performance.now() - start).toBeGreaterThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });
      });
    });

    describe('non-blocking calls', () => {
      describe('exclusive', () => {
        it('should not block and return true when another process does not have an exclusive lock', async () => {
          const start = performance.now();

          expect(Flock.exclusiveNB(F.fd)).toBe(true);
          expect(performance.now() - start).toBeLessThan(100);
        });

        it('should not block and return false when another process has an shared lock', async () => {
          const start = performance.now();
          const share = childMessages.next('share');

          await expect(share).resolves.toEqual({
            value: 'share',
            done: false
          });

          const unlocked = childMessages.next('unlock-later');

          expect(Flock.exclusiveNB(F.fd)).toBe(false);
          expect(performance.now() - start).toBeLessThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should not block and return false when another process has an exclusive lock', async () => {
          const start = performance.now();
          const share = childMessages.next('exclusive');

          await expect(share).resolves.toEqual({
            value: 'exclusive',
            done: false
          });

          const unlocked = childMessages.next('unlock-later');

          expect(Flock.exclusiveNB(F.fd)).toBe(false);
          expect(performance.now() - start).toBeLessThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });
      });

      describe('shared', () => {
        it('should not block and return true when another process does not have a lock', async () => {
          const start = performance.now();
          const unlocked = childMessages.next('unlock-later');

          expect(Flock.shareNB(F.fd)).toBe(true);
          expect(performance.now() - start).toBeLessThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should not block and return true when another process has a shared lock', async () => {
          const start = performance.now();
          const shared = childMessages.next('share');

          await expect(shared).resolves.toEqual({
            value: 'share',
            done: false
          });

          expect(Flock.shareNB(F.fd)).toBe(true);
          expect(performance.now() - start).toBeLessThan(100);

          const unlocked = childMessages.next('unlock');
          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should not block and return false when another process has an exclusive lock', async () => {
          const start = performance.now();
          const share = childMessages.next('exclusive');

          await expect(share).resolves.toEqual({
            value: 'exclusive',
            done: false
          });

          expect(Flock.shareNB(F.fd)).toBe(false);
          expect(performance.now() - start).toBeLessThan(100);
        });
      });
    });
  });
});
