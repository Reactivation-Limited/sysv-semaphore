const debug = require('debug')('parent');

module.exports = async function* generator(child) {
  let resolve;
  let reject;
  let waiting;

  process.on('SIGINT', () => {
    child.kill('SIGINT');
  });

  debug('generator begin');

  try {
    child.on('message', (message) => {
      try {
        if (waiting) {
          debug('rx', message);
          resolve(message);
        } else {
          throw 'unexpected message' + JSON.stringify(message);
        }
      } catch (whatever) {
        debug('generator throw');
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
      const command = yield waiting;
      debug('generator waiting for result from', command);
      if (command) {
        child.send(command);
      }
    }
  } finally {
    debug('generator finally');
    child.kill('SIGINT');
  }
};
