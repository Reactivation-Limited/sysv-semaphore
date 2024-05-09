const { Semaphore } = require('../build/Release/OSX.node');

console.log(Semaphore);


describe('Semaphore', () => {
  it('should be a class', () => {
    expect(new Semaphore).toBeInstanceOf(Semaphore);
  })
})
