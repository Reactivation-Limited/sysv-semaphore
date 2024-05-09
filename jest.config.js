export default {
  setupFiles: ['<rootDir>/jest-setup.js'],
  collectCoverage: true,
  collectCoverageFrom: ['<rootDir>/src/*/**', '!**/*.spike.js'],
  coverageThreshold: {
    global: {
      lines: 100
    }
  }
};
