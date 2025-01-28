import { Stats } from "fs";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "../watcher";
import { logger } from "@/logger";
import { extname } from "path";

export class OnAddService {
  execute({ self, path, stats }: TProps) {
    try {
      if (extname(path) === "") return;

      const { size, birthtime, mtime } = stats;
      if (size === 0 || size > 20 * 1024 * 1024 * 1024) return;

      const fileIntenty = self.virtualDriveFn.CfGetPlaceHolderIdentity(path);
      const status = self.virtualDriveFn.CfGetPlaceHolderState(path);

      const creationTime = new Date(birthtime).getTime();
      const modificationTime = new Date(mtime).getTime();

      let isNewFile = false;
      let isMovedFile = false;

      if (!fileIntenty) {
        // El archivo fue creado recientemente (dentro de los últimos 60 segundos)
        isNewFile = true;
      } else if (creationTime !== modificationTime) {
        // El archivo fue movido (o modificado)
        isMovedFile = true;
      }

      self.writeLog({
        event: "onAdd",
        path,
        fileIntenty,
        size,
        birthtime,
        mtime,
        status,
        isNewFile,
        isMovedFile,
      });

      // Procesar el archivo según su estado
      if (status.pinState === PinState.AlwaysLocal || status.pinState === PinState.OnlineOnly || status.syncState === SyncState.InSync) {
        return;
      }

      if (isNewFile) {
        self.fileInDevice.add(path);
        self.queueManager.enqueue({ path, type: typeQueue.add, isFolder: false });
      } else if (isMovedFile) {
        // Procesar archivo movido según sea necesario
      }
    } catch (error) {
      logger.error(error, "onAdd");
    }
  }
}

type TProps = {
  self: Watcher;
  path: string;
  stats: Stats;
};
