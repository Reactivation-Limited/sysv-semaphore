const debug = require('debug')('parent');

module.exports = async function* generator(child) {
  let resolve;
  let reject;
  let waiting;

  process.on('SIGINT', () => {
    child.kill('SIGINT');
  });

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
    child.kill('SIGINT');
  }
};
