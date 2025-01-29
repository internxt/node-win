import { getInfoItem } from "examples/info-items-manager";

import { sleep } from "@/utils";

type CallbackResponse = (data: boolean, path: string, errorHandler?: () => void) => Promise<{ finished: boolean; progress: number }>;

export const fetchDataCallback = async (id: string, callback: CallbackResponse) => {
  const path = await getInfoItem(id);

  let finish = false;
  while (!finish) {
    const result = await callback(true, path);
    finish = result.finished;
    await sleep(1000);
  }
};
