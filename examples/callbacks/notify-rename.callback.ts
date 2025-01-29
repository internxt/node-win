import { logger } from "@/logger";

export const notifyRenameCallback = (newName: string, fileId: string, callback: (response: boolean) => void) => {
  logger.info({ event: "notifyRenameCallback", newName, fileId });
  callback(true);
};
