module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  testMatch: ['**/test/**/*.test.ts'],
  moduleFileExtensions: ['ts', 'tsx', 'js', 'jsx', 'json', 'node'],
   moduleNameMapper: {
    "^examples/(.*)$": "<rootDir>/examples/$1", // Ajusta la ruta según sea necesario
  },
};
