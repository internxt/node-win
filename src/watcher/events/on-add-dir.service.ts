import { Stats } from "fs";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "../watcher";

export class OnAddDirService {
  execute({ self, path }: TProps) {
    try {
      const status = self.addon.getPlaceholderState({ path });
      self.logger.debug({ msg: "onAddDir", path, status });

      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      ) {
        return;
      }

      self.queueManager.enqueue({ path, type: typeQueue.add, isFolder: true });
    } catch (error) {
      self.logger.error({ msg: "Error en onAddDir", error });
    }
  }
}

type TProps = {
  self: Watcher;
  path: string;
  stats: Stats;
};
