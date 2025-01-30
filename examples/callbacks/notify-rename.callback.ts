import { logger } from "examples/drive";

export const notifyRenameCallback = (newName: string, fileId: string, callback: (response: boolean) => void) => {
  logger.info({ event: "notifyRenameCallback", newName, fileId });
  callback(true);
};
