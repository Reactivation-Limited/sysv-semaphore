const { open } = require('node:fs');
const o = require('../build/Debug/OSX');

try {
  o.Flock.unlock(65536);
} catch (error) {
  console.log(error.constructor);
  console.log(Object.getOwnPropertyDescriptors(error));
  console.log(Object.getOwnPropertyDescriptors(Object.getPrototypeOf(error)));
  console.log(error);
}

open('no-exist', 'r', (error) => {
  console.log(error.constructor);
  console.log(Object.getOwnPropertyDescriptors(error));
  console.log(Object.getOwnPropertyDescriptors(Object.getPrototypeOf(error)));
  console.log(error);
});
