import { logger } from "examples/drive";
import { getInfoItem } from "examples/info-items-manager";
import { rm } from "fs/promises";

import { TNotifyDeleteCallback } from "@/types/callbacks.type";

export const notifyDeleteCallback: TNotifyDeleteCallback = async (id, callback) => {
  const path = await getInfoItem({ id });
  await rm(path);

  logger.debug({
    msg: "notifyDeleteCallback",
    path,
    id,
  });

  callback(true);
};
