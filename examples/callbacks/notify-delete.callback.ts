import { logger } from "examples/drive";

export const notifyDeleteCallback = (fileId: string, callback: (response: boolean) => void) => {
  logger.info({ event: "notifyDeleteCallback", fileId });
  callback(true);
};
