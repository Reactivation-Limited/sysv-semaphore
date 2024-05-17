expect.extend({
  toThrowErrnoError(received, syscall, code) {
    const options = {
      comment: 'must be an Error with code, errno and syscall properties',
      isNot: false,
      promise: this.promise,
      secondArgument: code,
      secondArgumentColor: (arg) => arg
    };

    if (received instanceof Function) {
      try {
        received();
      } catch (error) {
        received = error;
      }
    }

    const messageRE = new RegExp(`^${code}: [^,]+, ${syscall}(?: .*)?`);

    // an Error of this "type" has this shape, in addition to the usual Error properties:
    // the intention, of course, is to indicate to the user they have made a programming error
    // {
    //   errno: 9,
    //   code: EBADF,
    //   syscall: flock
    // }

    // @todo prototypes do not match becasue the addon is running in a different isolate
    // see https://github.com/nodejs/node-addon-api/issues/743#issuecomment-662115592
    // also https://nodejs.org/api/n-api.html#napi_new_instance
    // work around: just test the name matches instead. not bullet proof, but good enough for now
    // if (!(received instanceof Error)) {
    if (received?.constructor?.name !== 'Error') {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, 'Error', options) +
          '\n\n' +
          `Expected: constructor ${this.utils.printExpected(Error)}\n` +
          `Received: constructor ${this.utils.printReceived(received?.constructor)}`,
        pass: false
      };
    } else if (received?.code !== code) {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, syscall, options) +
          '\n\n' +
          `Expected: code property ${this.utils.printExpected(code)}\n` +
          `Received: code property ${this.utils.printReceived(received?.code)}`,
        pass: false
      };
    } else if (typeof received?.errno !== 'number') {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, syscall, options) +
          '\n\n' +
          `Expected: errno property ${this.utils.printExpected(expect.any(Number))}\n` +
          `Received: ${this.utils.printReceived(received?.errno)}`,
        pass: false
      };
    } else if (received?.errno !== Math.floor(received?.errno)) {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, syscall, options) +
          '\n\n' +
          `Expected: errno property to be an integer\n` +
          `Received: ${this.utils.printReceived(received?.errno)}`,
        pass: false
      };
    } else if (received?.errno <= 0) {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, syscall, options) +
          '\n\n' +
          `Expected: errno property to be greater than 0\n` +
          `Received: ${this.utils.printReceived(received?.errno)}`,
        pass: false
      };
    } else if (received?.syscall !== syscall) {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, syscall, options) +
          '\n\n' +
          `Expected: syscall property ${this.utils.printExpected(syscall)}\n` +
          `Received: syscall property ${this.utils.printReceived(received?.syscall)}`,
        pass: false
      };
    } else if (!messageRE.test(received?.message)) {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, syscall, options) +
          '\n\n' +
          `Expected: message property matching ${this.utils.printExpected(messageRE)}\n` +
          `Received: message property ${this.utils.printReceived(received?.message)}`,
        pass: false
      };
    } else {
      return {
        message: () =>
          this.utils.matcherHint('toThrowErrnoError', received, syscall, options) +
          '\n\n' +
          `Expected: not ${this.utils.printExpected(Error)}\n` +
          `Received: ${this.utils.printReceived(received)}`,
        pass: true
      };
    }
  }
});
