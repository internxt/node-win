import { logger } from "examples/drive";
import { getInfoItem } from "examples/info-items-manager";

import { sleep } from "@/utils";

type TCallback = (data: boolean, path: string, errorHandler?: () => void) => Promise<{ finished: boolean; progress: number }>;

export const fetchDataCallback = async (id: string, callback: TCallback) => {
  logger.info({ fn: "fetchDataCallback", id });
  const path = await getInfoItem(id);

  let finish = false;
  while (!finish) {
    const result = await callback(true, path);
    finish = result.finished;
    await sleep(1000);
  }
};
