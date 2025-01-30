import { logger } from "examples/drive";

export const cancelFetchDataCallback = (fileId: string) => {
  logger.info({ event: "cancelFetchDataCallback", fileId });
};
