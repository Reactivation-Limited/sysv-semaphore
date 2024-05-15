const { open, unlink } = require('node:fs/promises');
const { fork } = require('node:child_process');

const { SemaphoreV: Semaphore } = require('../build/Debug/OSX.node');
const childMessages = require('../test/parent.js');

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
    console.log('refs', s.refs());
    s2.close();
    console.log('refs', s.refs());
    s.close(); // should unlink it

    console.log('child sem stuff');
    const s3 = Semaphore.createExclusive(name, 0o600, 1);
    console.log('parent', s3.refs());
    const child = fork('./test/semaphore-sysv-child.js', ['child'], {
      stdio: [process.stdin, process.stdout, process.stderr, 'ipc'],
      env: { DEBUG_COLORS: '', DEBUG: process.env.DEBUG }
    });

    process.on('SIGINT', () => {
      child.kill('SIGINT');
    });

    const messages = childMessages(child);
    const { value } = await messages.next();
    if (value !== name) {
      console.log('parent', name, 'child', value);
    }
    console.log('parent', s3.refs());
    child.kill('SIGTERM');
    await new Promise((resolve) => setTimeout(resolve, 1000));
    console.log('parent', s3.refs());
    s3.close();
  } catch (e) {
    console.log('error', e);
  } finally {
    console.log('unlink');
    try {
      Semaphore.unlink(name);
    } catch (error) {
      if (error.message === 'ENOENT') {
        console.log('already unlinked');
      } else {
        console.log(error);
      }
    }
    await F.close();
    await unlink(name);
  }
};

test();
