const { unlinkSync } = require('node:fs');
const { openSync, closeSync } = require('node:fs');

const { Flock } = require('..');

const debug = require('debug')('flock-child-process');

// the child always decides on the file
const path = './tmp/test-flock-child-' + process.pid;
const F = openSync(path, 'wx+');

const send = (...args) => {
  debug('child state is', ...args);
  process.send(...args);
};

const commands = {
  echo: (message) => message,
  share: () => {
    Flock.share(F);
    send('share');
  },
  exclusive: () => {
    Flock.exclusive(F);
    send('exclusive');
  },
  'unlock-later': () => {
    debug('unlock in 100ms');
    setTimeout(commands.unlock, 100);
  },
  unlock: () => {
    Flock.unlock(F);
    debug('unlocked');
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
  } catch (whatever) {
    send(whatever);
  }
});

send(path);

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
