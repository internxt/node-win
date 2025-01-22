import settings from "examples/settings";
import { join } from "path";
import { v4 } from "uuid";

export const generateFile = () => {
  const fileId = v4();
  const fileName = `file_${fileId}.txt`;
  const fullPath = join(settings.syncRootPath, fileName);
  const fileSize = 1000;
  const createdAt = Date.now();
  const updatedAt = Date.now() + 2000;

  return {
    fileId,
    fullPath,
    fileSize,
    createdAt,
    updatedAt,
  };
};
