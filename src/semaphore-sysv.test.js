const { fork } = require('node:child_process');
const { open, unlink } = require('node:fs/promises');
const childMessages = require('../test/parent.js');
const { SemaphoreV: Semaphore } = require('../build/Release/OSX.node');

const name = './tmp/semaphore-sysv';

describe('Semaphore', () => {
  beforeAll(async () => {
    const F = await open(name, 'wx');
    F.close();
  });
  afterAll(async () => {
    await unlink(name);
  });

  describe('open, create and unlink', () => {
    // order matters
    it('unlink should throw if the semaphore does not exist', () => {
      expect(() => Semaphore.unlink(name)).toThrow('ENOENT');
    });
    it('open should also throw if the semaphore does not exist', () => {
      expect(() => Semaphore.open(name)).toThrow('ENOENT');
    });
    it('create should create a semaphore if it does not exist', () => {
      expect(() => Semaphore.create(name, 1)).not.toThrow();
    });
    it('create should open a semaphore that already exists', () => {
      expect(() => Semaphore.create(name, 1)).not.toThrow();
    });
    it('open should open a semaphore that already exists', () => {
      expect(() => Semaphore.open(name)).not.toThrow();
    });
    it('unlink should not throw if the semaphore exists', () => {
      expect(() => Semaphore.unlink(name)).not.toThrow();
    });
  });

  describe('sempahore operations', () => {
    let semaphore;
    beforeAll(() => {
      semaphore = Semaphore.create(name, 1);
    });
    afterAll(() => {
      Semaphore.unlink(name);
    });

    // order matters
    it('should should decrement the semaphore without blocking', () => {
      expect(() => semaphore.wait()).not.toThrow();
    });

    it('should increment the semaphore without blocking', () => {
      expect(() => semaphore.post()).not.toThrow();
    });

    it('should should decrement the semaphore and return true without blocking', () => {
      expect(semaphore.trywait()).toBe(true);
    });

    it('should not decrement the semaphore and return false without blocking', () => {
      expect(semaphore.trywait()).toBe(false);
    });

    it('should increment the semaphore without blocking', () => {
      expect(() => semaphore.post()).not.toThrow();
    });
  });

  describe('process cooperation', () => {
    let semaphore;
    let messages;
    let child;
    beforeAll(async () => {
      child = fork('./test/semaphore-sysv-child.js', ['child'], {
        stdio: [process.stdin, process.stdout, process.stderr, 'ipc'],
        env: { DEBUG_COLORS: '', DEBUG: process.env.DEBUG }
      });

      process.on('SIGINT', () => {
        child.kill('SIGINT');
      });

      const name = Buffer.from('flock-child-' + child.pid).toString('base64');

      messages = childMessages(child);
      await expect(messages.next()).resolves.toEqual({ done: false, value: name });
      semaphore = Semaphore.open(name);
    });

    afterAll(async () => {
      child.kill();
      await new Promise((resolve) => child.once('close', resolve));
    });

    describe('non blocking calls', () => {
      it('trywait should return false when the parent has the semaphore', async () => {
        expect(semaphore.trywait()).toBe(true);
        await expect(messages.next('trywait')).resolves.toEqual({
          value: 'not aquired count=0',
          done: false
        });
      });
      it('trywait should return true when the parent does not have the semaphore', async () => {
        expect(() => semaphore.post()).not.toThrow();
        await expect(messages.next('trywait')).resolves.toEqual({
          value: 'aquired count=1',
          done: false
        });
      });
    });
    describe('blocking calls', () => {
      it('trywait should return true when the child does not have the semaphore', async () => {
        await expect(messages.next('post')).resolves.toEqual({
          value: 'released count=0',
          done: false
        });
        expect(semaphore.trywait()).toBe(true);
      });
      it('wait should pause the child until the semaphore is released by the parent', async () => {
        const start = performance.now();
        await expect(messages.next('wait')).resolves.toEqual({
          value: 'waiting',
          done: false
        });
        setTimeout(() => semaphore.post(), 10);
        await expect(messages.next()).resolves.toEqual({
          value: 'aquired count=1',
          done: false
        });
        expect(performance.now() - start).toBeGreaterThan(10);
      });
      it('trywait should return false when the child does have the semaphore', async () => {
        const start = performance.now();
        expect(semaphore.trywait()).toBe(false);
        expect(performance.now() - start).toBeLessThan(10);
      });
      it('trywait should not pause the child until the semaphore is released by the parent', async () => {
        const start = performance.now();
        await expect(messages.next('trywait')).resolves.toEqual({
          value: 'not aquired count=1',
          done: false
        });
        expect(performance.now() - start).toBeLessThan(10);
        semaphore.post();
      });
    });
  });
});
