import js from '@eslint/js';
import jest from 'eslint-plugin-jest';
import globals from 'globals';

const jestGlobals = Object.keys(jest.environments.globals.globals).reduce(
  (object, key) =>
    Object.defineProperty(object, key, {
      writable: false,
      configurable: false,
      enumerable: true,
      value: 'readonly'
    }),
  {}
);

export default [
  js.configs.recommended,
  {
    files: ['**/*.js'],
    languageOptions: {
      ecmaVersion: 'latest',
      sourceType: 'module',
      globals: {
        ...globals.node
      }
    },
    rules: {
      'no-unused-vars': [
        'error',
        {
          varsIgnorePattern: '^_'
        }
      ]
    }
  },
  {
    files: ['**/*.test.js', 'jest-matchers.js', 'test/**/*.js'],
    plugins: {
      jest: jest
    },
    languageOptions: {
      globals: {
        ...jestGlobals,
        GeneratorFunction: 'readonly',
        AsyncFunction: 'readonly',
        AsyncGeneratorFunction: 'readonly',
        Generator: 'readonly',
        AsyncGenerator: 'readonly'
      }
    },
    rules: {
      'no-console': 'error'
    }
  }
];
