import { Stats } from "fs";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "../watcher";

export class OnAddService {
  execute({ self, path, stats }: TProps) {
    try {
      const ext = path.split(".").pop();
      const { size, birthtime, mtime } = stats;
      const itemId = self.virtualDriveFn.CfGetPlaceHolderIdentity(path);

      self.logger.info({ fn: "onAdd", path, ext, size, birthtime, mtime, itemId });

      if (!ext || size === 0 || size > 20 * 1024 * 1024 * 1024) return;

      const status = self.virtualDriveFn.CfGetPlaceHolderState(path);
      self.logger.info({ fn: "onAdd", path, status });

      // Verificar tiempos de creación y modificación
      const creationTime = new Date(birthtime).getTime();
      const modificationTime = new Date(mtime).getTime();
      const currentTime = Date.now();

      let isNewFile = false;
      let isMovedFile = false;

      if (!itemId) {
        // El archivo fue creado recientemente (dentro de los últimos 60 segundos)
        isNewFile = true;
      } else if (creationTime !== modificationTime) {
        // El archivo fue movido (o modificado)
        isMovedFile = true;
      }

      // Procesar el archivo según su estado
      if (status.pinState === PinState.AlwaysLocal || status.pinState === PinState.OnlineOnly || status.syncState === SyncState.InSync) {
        return;
      }

      if (isNewFile) {
        self.fileInDevice.add(path);
        self.queueManager.enqueue({ path, type: typeQueue.add, isFolder: false });
      } else if (isMovedFile) {
        self.logger.info({ fn: "onAdd", msg: "File moved", path });
        // Procesar archivo movido según sea necesario
      }
    } catch (error) {
      self.logger.error("onAddService", error);
    }
  }
}

type TProps = {
  self: Watcher;
  path: string;
  stats: Stats;
};
