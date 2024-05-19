const { SemaphoreP } = require('../build/Debug/OSX.node');
// const { SemaphoreP, o_creat, o_excl } = require('./src/index.js');
const name = 'semaphore-test'; // 'E8A102C85C194B3C9C1E03975241';

try {
  console.log('create');
  SemaphoreP.createShared(name, 0o600, 10);
} catch (e) {
  console.log('error', e);
}
try {
  console.log('create exclusive');
  SemaphoreP.createExclusive(name, 0o600, 10);
} catch (e) {
  if (e.message !== 'EEXIST') {
    console.log('error', e);
  }
}
let s;
try {
  console.log('open');
  s = SemaphoreP.open(name);
} catch (e) {
  console.log('error', e);
}
try {
  console.log('close');
  s.close();
} catch (e) {
  console.log('error', e);
}
try {
  console.log('unlink');
  SemaphoreP.unlink(name);
} catch (e) {
  console.log('error', e);
}
try {
  console.log('create exclusive');
  SemaphoreP.createExclusive(name, 0o600, 10);
} catch (e) {
  console.log('error', e);
}
try {
  console.log('unlink');
  SemaphoreP.unlink(name);
} catch (e) {
  console.log('error', e);
}
