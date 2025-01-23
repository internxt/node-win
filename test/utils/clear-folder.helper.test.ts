import { readdir, rmdir, stat, unlink } from "fs/promises";
import { join } from "path";

export const clearFolder = async (path: string) => {
  const files = await readdir(path);

  for (const file of files) {
    const filePath = join(path, file);
    const stats = await stat(filePath);

    if (stats.isDirectory()) {
      await clearFolder(filePath);
      await rmdir(filePath);
    } else {
      await unlink(filePath);
    }
  }
};
