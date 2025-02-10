import { Stats } from "fs";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "../watcher";

export class OnAddService {
  execute({ self, path, stats }: { self: Watcher; path: string; stats: Stats }) {
    try {
      const { size, birthtime, mtime } = stats;

      if (size === 0 || size > 20 * 1024 * 1024 * 1024) return;

      const itemId = self.addon.getFileIdentity({ path });
      const status = self.addon.getPlaceholderState({ path });

      self.logger.info({ fn: "onAdd", path, size, birthtime, mtime, itemId, status });

      const creationTime = new Date(birthtime).getTime();
      const modificationTime = new Date(mtime).getTime();

      let isNewFile = false;
      let isMovedFile = false;

      if (!itemId) {
        isNewFile = true;
      } else if (creationTime !== modificationTime) {
        isMovedFile = true;
      }

      if (status.pinState === PinState.AlwaysLocal || status.pinState === PinState.OnlineOnly || status.syncState === SyncState.InSync) {
        return;
      }

      if (isNewFile) {
        self.fileInDevice.add(path);
        self.queueManager.enqueue({ path, type: typeQueue.add, isFolder: false });
      } else if (isMovedFile) {
        self.logger.info({ fn: "onAdd", msg: "File moved", path });
      }
    } catch (error) {
      self.logger.error("onAddService", error);
    }
  }
}
