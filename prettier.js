module.exports = {
  arrowParens: "always",
  bracketSameLine: true,
  bracketSpacing: true,
  endOfLine: "crlf",
  importOrder: ["^@/(.*)$", "^[./]"],
  importOrderParserPlugins: ["typescript", "decorators-legacy"],
  importOrderSeparation: true,
  plugins: [require.resolve("@trivago/prettier-plugin-sort-imports")],
  printWidth: 140,
  proseWrap: "never",
  semi: true,
  singleQuote: true,
  tabWidth: 2,
  trailingComma: "all",
  useTabs: false,
};
