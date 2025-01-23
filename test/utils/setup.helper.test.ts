import { mkdirSync, rmSync } from "fs";

export const TEST_FILES = "test-files";

rmSync(TEST_FILES, { recursive: true, force: true });
mkdirSync(TEST_FILES, { recursive: true });
