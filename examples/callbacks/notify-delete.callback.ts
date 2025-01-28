import { logger } from "@/logger";

export const onDeleteCallback = (fileId: string, callback: (response: boolean) => void) => {
  logger.info({ event: "onDelete", fileId });
  callback(true);
};
