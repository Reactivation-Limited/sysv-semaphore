const { fork } = require('node:child_process');
const { open, unlink } = require('node:fs/promises');
const childMessages = require('./parent.js');
const { SemaphoreV: Semaphore, Token } = require('..');

const name = './tmp/semaphore-sysv';

describe('Semaphore', () => {
  let key;

  beforeAll(async () => {
    const F = await open(name, 'wx');
    F.close();
    key = new Token(name, 0);
  });
  afterAll(async () => {
    await unlink(name);
  });

  describe('open, create and unlink', () => {
    // order matters
    afterAll(() => {
      expect(() => Semaphore.unlink(key)).not.toThrow();
    });
    it('unlink should throw if the semaphore does not exist', () => {
      expect(() => Semaphore.unlink(key)).toThrowErrnoError('semget', 'ENOENT');
    });
    it('open should also throw if the semaphore does not exist', () => {
      expect(() => Semaphore.open(key)).toThrowErrnoError('semget', 'ENOENT');
    });
    it('create should create a semaphore if it does not exist', () => {
      expect(() => Semaphore.create(key, 0o600, 1)).not.toThrow();
    });
    it('createExclusive should throw if the semaphore already exists', () => {
      expect(() => Semaphore.createExclusive(key, 0o600, 1)).toThrowErrnoError('semget', 'EEXIST');
    });
    it('create should open a semaphore that already exists', () => {
      expect(() => Semaphore.create(key, 0o600, 1)).not.toThrow();
    });
    it('open should open a semaphore that already exists', () => {
      expect(() => Semaphore.open(key)).not.toThrow();
    });
    it('unlink should not throw if the semaphore exists', () => {
      expect(() => Semaphore.unlink(key)).not.toThrow();
    });
    it('createExclusive should create a semaphore if it does not already exist', () => {
      expect(() => Semaphore.createExclusive(key, 0o600, 1)).not.toThrow();
    });
  });

  describe('close', () => {
    afterAll(() => {
      expect(() => Semaphore.unlink(key)).toThrow('ENOENT');
    });
    it('should unlink the semaphore if this is the last reference', () => {
      const semaphore = Semaphore.createExclusive(key, 0o600, 1);
      semaphore.close();
      expect(() => semaphore.wait()).toThrowErrnoError('semop', 'EINVAL');
      expect(() => semaphore.trywait()).toThrowErrnoError('semop', 'EINVAL');
      expect(() => semaphore.post()).toThrowErrnoError('semop', 'EINVAL');
      expect(() => semaphore.close()).toThrowErrnoError('semop', 'EINVAL');
    });
    it('should not unlink the semaphore if this is not the last reference', () => {
      const semaphore = Semaphore.createExclusive(key, 0o600, 1);
      Semaphore.create(key, 0o600, 1);
      semaphore.close();
      expect(() => Semaphore.unlink(key)).not.toThrow();
    });
    it('should not unlink the semaphore if this is not the last reference', () => {
      const semaphore = Semaphore.createExclusive(key, 0o600, 1);
      Semaphore.open(key);
      semaphore.close();
      expect(() => Semaphore.unlink(key)).not.toThrow();
    });
  });

  describe('sempahore operations', () => {
    let semaphore;
    beforeAll(() => {
      semaphore = Semaphore.createExclusive(key, 0o600, 10);
    });
    afterAll(() => {
      Semaphore.unlink(key);
    });

    it('should have the initial value of 10', () => {
      expect(semaphore.valueOf()).toBe(10);
    });

    // order matters
    it('should should throw if the wait argument is negative', () => {
      expect(() => semaphore.wait(-1)).toThrow('Illegal arguments for function wait.');
    });

    it('should should subtract 9 from the semaphore without blocking', () => {
      expect(() => semaphore.wait(9)).not.toThrow();
      expect(semaphore.valueOf()).toBe(1);
    });

    it('should should decrement the semaphore without blocking', () => {
      expect(() => semaphore.wait()).not.toThrow();
      expect(semaphore.valueOf()).toBe(0);
    });

    it('should increment the semaphore', () => {
      expect(() => semaphore.post()).not.toThrow();
      expect(semaphore.valueOf()).toBe(1);
    });

    it('should add 9 to the semaphore', () => {
      expect(() => semaphore.post(9)).not.toThrow();
      expect(semaphore.valueOf()).toBe(10);
    });

    it('should should decrement the semaphore and return true without blocking', () => {
      expect(semaphore.trywait()).toBe(true);
      expect(semaphore.valueOf()).toBe(9);
    });

    it('should not subtract 10 from the semaphore and return false without blocking', () => {
      expect(semaphore.trywait(10)).toBe(false);
      expect(semaphore.valueOf()).toBe(9);
    });

    it('should subtract 9 from the semaphore and return true without blocking', () => {
      expect(semaphore.trywait(9)).toBe(true);
      expect(semaphore.valueOf()).toBe(0);
    });

    it('should not decrement the semaphore and return false without blocking', () => {
      expect(semaphore.trywait()).toBe(false);
      expect(semaphore.valueOf()).toBe(0);
    });

    it('should increment the semaphore without blocking', () => {
      expect(() => semaphore.post()).not.toThrow();
      expect(semaphore.valueOf()).toBe(1);
    });

    it('should add 10 to the semaphore without blocking', () => {
      expect(() => semaphore.post(10)).not.toThrow();
      expect(semaphore.valueOf()).toBe(11);
    });
  });

  describe('process cooperation', () => {
    let semaphore;
    let messages;
    let child;
    beforeAll(async () => {
      semaphore = Semaphore.createExclusive(key, 0o600, 1);
      child = fork('./test/semaphore-sysv-child.js', [name], {
        stdio: [process.stdin, process.stdout, process.stderr, 'ipc'],
        env: { DEBUG_COLORS: '', DEBUG: process.env.DEBUG }
      });
      process.on('SIGINT', () => {
        child.kill('SIGINT');
      });
      messages = childMessages(child);
      await expect(messages.next()).resolves.toEqual({ done: false, value: name });
    });

    afterAll(async () => {
      child.kill('SIGKILL');
      await new Promise((resolve) => {
        child.once('exit', (code, signal) => {
          code ? resolve(code) : resolve(signal);
        });
      });
      semaphore.close();
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

  // Note that there is a corner case when the last process crashes and cannot unlink the sem
  // this is a minor resource leak, and will be corrected when the the next group of processes runs
  it('should have automatically removed the semaphore when the last process closed it', () => {
    expect(() => Semaphore.open(key)).toThrowErrnoError('semget', 'ENOENT');
  });
});
