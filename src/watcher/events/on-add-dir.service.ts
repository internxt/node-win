import { Stats } from "fs";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "../watcher";

export class OnAddDirService {
  execute({ self, path, stats }: TProps) {
    try {
      self.writeLog("onAddDir", path, stats);

      const status = self.virtualDriveFn.CfGetPlaceHolderState(path);
      self.writeLog("status", status);

      if (status.pinState === PinState.AlwaysLocal || status.pinState === PinState.OnlineOnly || status.syncState === SyncState.InSync) {
        return;
      }

      self.queueManager.enqueue({ path, type: typeQueue.add, isFolder: true });
    } catch (error) {
      self.writeLog("Error en onAddDir");
      console.error(error);
    }
  }
}

type TProps = {
  self: Watcher;
  path: string;
  stats: Stats;
};
