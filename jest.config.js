module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  testMatch: ['**/test/**/*.e2e.test.ts'],
  setupFiles: ["<rootDir>/test/utils/setup.helper.test.ts"],
   moduleNameMapper: {
    "^examples/(.*)$": "<rootDir>/examples/$1",
    "^test/(.*)$": "<rootDir>/test/$1",
    "^@/(.*)$": "<rootDir>/src/$1",
  },
  coverageThreshold: {
    global: {
      branches: 70,
      functions: 70,
      lines: 70,
      statements: 70,
    },
  },
};
