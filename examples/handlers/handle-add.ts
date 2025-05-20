import { drive, logger } from "examples/drive";
import { addInfoItem } from "examples/info-items-manager";

import { QueueItem } from "@/queue/queueManager";

export const handleAdd = async (task: QueueItem) => {
  try {
    logger.debug({ msg: "handleAdd", task });

    const id = addInfoItem({
      itemPath: task.path,
      isFile: !task.isFolder,
    });

    drive.convertToPlaceholder({
      itemPath: task.path,
      id,
    });
  } catch (error) {
    logger.error({ msg: "handleAdd", error });
  }
};
