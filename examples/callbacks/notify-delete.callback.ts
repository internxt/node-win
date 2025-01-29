import { logger } from "@/logger";

export const notifyDeleteCallback = (fileId: string, callback: (response: boolean) => void) => {
  logger.info({ event: "onDelete", fileId });
  callback(true);
};
