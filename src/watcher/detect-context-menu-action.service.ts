import { typeQueue } from "@/queue/queueManager";
import { Attributes, PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "./watcher";

export class DetectContextMenuActionService {
  async execute({ self, details, path, isDirectory }: TProps) {
    const { prev, curr } = details;
    const status = await self.virtualDriveFn.CfGetPlaceHolderState(path);
    self.writeLog("status", status);

    const attribute: Attributes = await self.virtualDriveFn.CfGetPlaceHolderAttributes(path);
    self.writeLog("attribute", attribute);

    const itemId = self.virtualDriveFn.CfGetPlaceHolderIdentity(path);
    self.writeLog("itemId", itemId);

    const isInDevice = self.fileInDevice.has(path);

    if (
      prev.size === curr.size &&
      prev.ctimeMs !== curr.ctimeMs &&
      prev.mtimeMs === curr.mtimeMs &&
      status.pinState === PinState.AlwaysLocal &&
      status.syncState === SyncState.InSync &&
      !isInDevice
    ) {
      self.fileInDevice.add(path);
      self.queueManager.enqueue({
        path,
        type: typeQueue.hydrate,
        isFolder: isDirectory,
        fileId: itemId,
      });
      return "Mantener siempre en el dispositivo";
    }

    // Verificar si es "Liberar espacio"
    if (
      prev.size == curr.size && // Tamaño no cambia
      prev.ctimeMs != curr.ctimeMs && // ctime cambia
      status.pinState == PinState.OnlineOnly && // Estado es OnlineOnly
      status.syncState == SyncState.InSync // Estado es InSync
    ) {
      self.fileInDevice.delete(path);
      self.queueManager.enqueue({
        path,
        type: typeQueue.dehydrate,
        isFolder: isDirectory,
        fileId: itemId,
      });
      return "Liberar espacio";
    }

    if (prev.size != curr.size) {
      self.queueManager.enqueue({
        path,
        type: typeQueue.changeSize,
        isFolder: isDirectory,
        fileId: itemId,
      });
      self.fileInDevice.add(path);
      return "Cambio de tamaño";
    }

    return null;
  }
}

type TProps = {
  self: Watcher;
  details: any;
  path: string;
  isDirectory: boolean;
};
