import { logger } from "examples/drive";

export const notifyMessageCallback = (message: string, action: string, errorName: string, callback: (response: boolean) => void) => {
  logger.info({ event: "notifyMessageCallback", message, action, errorName });
  callback(true);
};
