const { unlinkSync } = require('node:fs');
const { openSync, closeSync } = require('node:fs');

const { Flock } = require('../build/Release/OSX.node');

const debug = require('debug')('flock-child-process');

// the child always decides on the file
const path = './tmp/test-flock-child-' + process.pid;
const F = openSync(path, 'wx+');

const send = (...args) => {
  debug('child tx', ...args);
  process.send(...args);
};

let state = 'unlocked';

const commands = {
  echo: (message) => message,
  share: () => {
    Flock.share(F);
    state = 'share';
    send('share');
  },
  exclusive: () => {
    Flock.exclusive(F);
    state = 'exclusive';
    send('exclusive');
  },
  'unlock-later': () => {
    setTimeout(() => {
      commands.unlock();
    }, 100);
  },
  unlock: () => {
    Flock.unlock(F);
    state = 'share';
    send('unlock');
  }
};

process.on('message', (message) => {
  debug('child rx', message);

  try {
    const command = commands[message];
    if (!command) {
      throw `no command ${message}`;
    }
    command();
    debug('child', state);
  } catch (whatever) {
    send(whatever);
  }
});

process.send(path);

const exit = (code) => {
  try {
    closeSync(F);
  } finally {
    unlinkSync(path);
  }
  process.exit(code);
};

setTimeout(() => {
  debug('child died of boredom');
  exit(1);
}, 5000);

process.on('SIGINT', () => {
  exit(0);
});

process.on('SIGTERM', () => {
  exit(0);
});
