import { mkdirSync, rmSync } from "fs";
import { join } from "path";
import { cwd } from "process";

export const TEST_FILES = join(cwd(), "test-files");

rmSync(TEST_FILES, { recursive: true, force: true });
mkdirSync(TEST_FILES, { recursive: true });
