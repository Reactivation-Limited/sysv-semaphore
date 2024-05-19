module.exports = {
  roots: ['<rootDir>/test'],
  setupFiles: ['<rootDir>/jest-setup.js'],
  setupFilesAfterEnv: ['<rootDir>/test/jest-matchers.js'],
  transform: {},
  collectCoverage: true,
  collectCoverageFrom: ['<rootDir>/src/*/**', '!**/*.spike.js'],
  coverageThreshold: {
    global: {
      lines: 100
    }
  }
};
