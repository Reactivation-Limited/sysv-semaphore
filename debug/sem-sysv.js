const { open, unlink } = require('node:fs/promises');
const { SemaphoreV: Semaphore } = require('../build/Debug/OSX.node');

console.log(Semaphore);

const name = './tmp/semaphore-sysv';

const test = async () => {
  let F;
  console.log('open', name);
  F = await open(name, 'wx');
  let s;
  try {
    console.log('sem open');
    s = Semaphore.createExclusive(name, 0o600, 1);
    const s2 = Semaphore.create(name, 0o600, 1);
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
    s2.close();
    s.close(); // should unlink it
    s.post();
  } catch (e) {
    console.log('error', e);
  } finally {
    console.log('unlink');
    try {
      Semaphore.unlink(name);
    } catch (error) {
      console.log('close did not unlink');
    }
    await F.close();
    await unlink(name);
  }
};

test();
