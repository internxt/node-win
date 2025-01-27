import { mkdirSync } from "fs";
import { join } from "path";
import { cwd } from "process";

export const TEST_FILES = join(cwd(), "test-files");

mkdirSync(TEST_FILES, { recursive: true });
