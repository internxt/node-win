import { getInfoItem } from "examples/info-items-manager";

import { TFetchDataCallback } from "@/types/callbacks.type";
import { sleep } from "@/utils";

export const fetchDataCallback = async (id: string, callback: Parameters<TFetchDataCallback>[1]) => {
  const path = await getInfoItem(id);

  let finish = false;
  while (!finish) {
    const result = await callback(true, path);
    finish = result.finished;
    await sleep(1000);
  }
};
