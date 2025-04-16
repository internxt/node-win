import { drive, logger } from "examples/drive";

import { QueueItem } from "@/queue/queueManager";

export const handleDehydrate = async (task: QueueItem) => {
  try {
    logger.debug({ msg: "handleDehydrate", path: task.path });
    drive.dehydrateFile({
      itemPath: task.path,
    });
  } catch (error) {
    logger.error({ msg: "handleDehydrate", error });
  }
};
