import js from "@eslint/js";
import { defineConfig, globalIgnores } from "eslint/config";
import globals from "globals";
import tseslint from "typescript-eslint";

export default defineConfig([
  { files: ["**/*.{js,mjs,cjs,ts}"], plugins: { js }, extends: ["js/recommended"] },
  { files: ["**/*.{js,mjs,cjs,ts}"], languageOptions: { globals: globals.node } },
  tseslint.configs.recommended,
  globalIgnores(["dist", ".prettierrc.js", "eslint.config.mjs"]),
  {
    languageOptions: {
      parserOptions: {
        project: ["tsconfig.json"],
      },
    },
    rules: {
      "@typescript-eslint/no-floating-promises": "error",
      "@typescript-eslint/no-explicit-any": "warn",
      "object-shorthand": "error",
    },
  },
]);
