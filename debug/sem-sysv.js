const { open, unlink } = require('node:fs/promises');
const { SemaphoreV: Semaphore } = require('../build/Debug/OSX.node');

console.log(Semaphore);

const name = './tmp/semaphore-sysv';

const test = async () => {
  let F;
  console.log('open', name);
  F = await open(name, 'wx');
  try {
    console.log('sem open');
    const s = Semaphore.createExclusive(name, 0o600, 1);
    if (!s.trywait()) {
      throw 'trywait false';
    }
    if (s.trywait()) {
      throw 'trywait true';
    }
    s.post();
    if (!s.trywait()) {
      throw 'trywait false';
    }
  } catch (e) {
    console.log('error', e);
  } finally {
    console.log('unlink');
    Semaphore.unlink(name);
    await F.close();
    await unlink(name);
  }
};

test();
