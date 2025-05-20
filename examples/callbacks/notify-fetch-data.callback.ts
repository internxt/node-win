import { logger } from "examples/drive";
import { getInfoItem } from "examples/info-items-manager";

import { TFetchDataCallback } from "@/types/callbacks.type";
import { sleep } from "@/utils";

export const fetchDataCallback: TFetchDataCallback = async (id, callback) => {
  logger.debug({ msg: "fetchDataCallback", id });
  const path = await getInfoItem({ id });

  let finish = false;
  while (!finish) {
    const result = await callback(true, path);
    finish = result.finished;
    await sleep(1000);
  }
};
