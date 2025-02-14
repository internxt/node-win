import { Stats } from "fs";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "./watcher";

export class DetectContextMenuActionService {
  async execute({ self, details, path, isFolder }: TProps) {
    const { prev, curr } = details;

    const status = self.addon.getPlaceholderState({ path });
    const itemId = self.addon.getFileIdentity({ path });
    const isInDevice = self.fileInDevice.has(itemId) || self.fileInDevice.has(path);

    self.logger.info({
      event: "change",
      path,
      status,
      itemId,
      isInDevice,
      prev: {
        size: prev.size,
        ctimeMs: prev.ctimeMs,
        mtimeMs: prev.mtimeMs,
      },
      curr: {
        size: curr.size,
        ctimeMs: curr.ctimeMs,
        mtimeMs: curr.mtimeMs,
      },
    });

    // TODO: check same size but different content
    if (prev.size !== curr.size) {
      self.queueManager.enqueue({ path, type: typeQueue.changeSize, isFolder, fileId: itemId });
      self.fileInDevice.add(itemId);
      return "Cambio de tamaño";
    }

    if (prev.ctimeMs !== curr.ctimeMs && status.syncState === SyncState.InSync) {
      if (status.pinState === PinState.AlwaysLocal && !isInDevice) {
        self.fileInDevice.add(itemId);
        self.queueManager.enqueue({ path, type: typeQueue.hydrate, isFolder, fileId: itemId });
        return "Mantener siempre en el dispositivo";
      }

      if (status.pinState == PinState.OnlineOnly && isInDevice) {
        self.fileInDevice.delete(path);
        self.fileInDevice.delete(itemId);
        self.queueManager.enqueue({ path, type: typeQueue.dehydrate, isFolder, fileId: itemId });
        return "Liberar espacio";
      }
    }
  }
}

type TProps = {
  self: Watcher;
  details: {
    prev: Stats;
    curr: Stats;
  };
  path: string;
  isFolder: boolean;
};
