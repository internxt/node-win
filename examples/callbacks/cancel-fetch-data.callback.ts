import { logger } from "@/logger";

export const cancelFetchDataCallback = (fileId: string) => {
  logger.info({ event: "cancelFetchDataCallback", fileId });
};
