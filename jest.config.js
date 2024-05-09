module.exports = {
  setupFiles: ['<rootDir>/jest-setup.js'],
  transform: {},
  collectCoverage: true,
  collectCoverageFrom: ['<rootDir>/src/*/**', '!**/*.spike.js'],
  coverageThreshold: {
    global: {
      lines: 100
    }
  }
};
