const { Semaphore, o_creat, o_excl } = require('../build/Release/OSX.node');

const name = 'semaphore-test';

console.log('Semaphore', Semaphore, o_creat, o_excl);

describe('Semaphore', () => {
  it('should be a class', () => {
    expect(new Semaphore()).toBeInstanceOf(Semaphore);
  });

  describe('sempahore open and close', () => {
    let semaphore;
    beforeEach(() => {
      semaphore = new Semaphore();
    });
    afterEach(() => {
      // expect(() => semaphore.unlink()).not.toThrow();
    });

    it.only('should open a semaphore', () => {
      expect(() => semaphore.open(name)).not.toThrow();
    });
  });
});
