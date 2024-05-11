const { Flock } = require('../build/Release/OSX.node');
const { mkdir, open, unlink } = require('node:fs/promises');
const { fork } = require('node:child_process');
const childMessages = require('../test/parent.js');
const debug = require('debug')('flock-test');

describe('Semaphore', () => {
  it('should be defined', () => {
    expect(Flock).not.toBe(undefined);
  });

  it('should not lock non-files', async () => {
    expect(() => Flock.share(-1)).toThrow('EBADF');
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
    let messages;
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

      messages = childMessages(child);
      const n = await messages.next();
      debug(n);
      file = n.value;
    });
    beforeEach(async () => {
      F = await open(file, 'r');
    });
    afterEach(async () => {
      await F.close();
    });
    afterAll(async () => {
      child.kill();
      await new Promise((resolve) => child.once('close', resolve));
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
          const share = messages.next('share');

          await expect(share).resolves.toEqual({
            value: 'share',
            done: false
          });

          const unlocked = messages.next('unlock-later');

          expect(() => Flock.exclusive(F.fd)).not.toThrow();
          expect(performance.now() - start).toBeGreaterThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should block and get an exclusive lock when another process releases an exclusive lock', async () => {
          const start = performance.now();
          const share = messages.next('exclusive');

          await expect(share).resolves.toEqual({
            value: 'exclusive',
            done: false
          });

          const unlocked = messages.next('unlock-later');

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
          const shared = messages.next('share');

          await expect(shared).resolves.toEqual({
            value: 'share',
            done: false
          });

          const unlocked = messages.next('unlock-later');

          expect(() => Flock.share(F.fd)).not.toThrow();
          expect(performance.now() - start).toBeLessThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should block and get a shared lock when another process has an exclusive lock', async () => {
          const start = performance.now();
          const share = messages.next('exclusive');

          await expect(share).resolves.toEqual({
            value: 'exclusive',
            done: false
          });

          const unlocked = messages.next('unlock-later');

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
          const share = messages.next('share');

          await expect(share).resolves.toEqual({
            value: 'share',
            done: false
          });

          const unlocked = messages.next('unlock-later');

          expect(Flock.exclusiveNB(F.fd)).toBe(false);
          expect(performance.now() - start).toBeLessThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should not block and return false when another process has an exclusive lock', async () => {
          const start = performance.now();
          const share = messages.next('exclusive');

          await expect(share).resolves.toEqual({
            value: 'exclusive',
            done: false
          });

          const unlocked = messages.next('unlock-later');

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
          const unlocked = messages.next('unlock-later');

          expect(Flock.shareNB(F.fd)).toBe(true);
          expect(performance.now() - start).toBeLessThan(100);

          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should not block and return true when another process has a shared lock', async () => {
          const start = performance.now();
          const shared = messages.next('share');

          await expect(shared).resolves.toEqual({
            value: 'share',
            done: false
          });

          expect(Flock.shareNB(F.fd)).toBe(true);
          expect(performance.now() - start).toBeLessThan(100);

          const unlocked = messages.next('unlock');
          await expect(unlocked).resolves.toEqual({
            value: 'unlock',
            done: false
          });
        });

        it('should not block and return false when another process has an exclusive lock', async () => {
          const start = performance.now();

          await expect(messages.next('exclusive')).resolves.toEqual({
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
