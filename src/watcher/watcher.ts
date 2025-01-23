import * as chokidar from "chokidar";
import { IVirtualDriveFunctions } from "./watcher.interface";
import {
  PinState,
  Status,
  SyncState,
  Attributes,
} from "../types/placeholder.type";
import { IQueueManager, typeQueue } from "../queue/queueManager";
import fs, { Stats } from "fs";
import * as Path from "path";
import { inspect } from "util";

export namespace Watcher {
  export type TOptions = chokidar.WatchOptions;
}

export class Watcher {
  private syncRootPath!: string;
  private options!: Watcher.TOptions;
  private virtualDriveFn!: IVirtualDriveFunctions;
  private queueManager!: IQueueManager;
  private logPath!: string;
  private fileInDevice = new Set<string>();

  init(
    queueManager: IQueueManager,
    syncRootPath: string,
    options: chokidar.WatchOptions,
    logPath: string,
    virtualDriveFn: IVirtualDriveFunctions
  ) {
    this.queueManager = queueManager;
    this.syncRootPath = syncRootPath;
    this.options = options;
    this.logPath = logPath;
    this.virtualDriveFn = virtualDriveFn;
  }

  public writeLog = (...messages: unknown[]) => {
    const date = new Date().toISOString();
    const parsedMessages = inspect(messages, { depth: Infinity });
    const logMessage = `${date} - ${parsedMessages}`;

    fs.appendFile(this.logPath, logMessage, (err) => {
      if (err) {
        console.error("Error writing to log file", err);
      }
    });
  };

  private onAdd = (path: string, state: any) => {
    console.log("onAdd")
    try {
      this.writeLog("onAdd", path, state);
      const ext = path.split(".").pop();

      const { size, birthtime, mtime } = state;

      const fileIntenty = this.virtualDriveFn.CfGetPlaceHolderIdentity(path);

      this.writeLog("fileIntenty in add", fileIntenty);

      if (!ext || size === 0 || size > 20 * 1024 * 1024 * 1024) return;

      const status: Status = this.virtualDriveFn.CfGetPlaceHolderState(path);
      this.writeLog("status", status);

      // Verificar tiempos de creación y modificación
      const creationTime = new Date(birthtime).getTime();
      const modificationTime = new Date(mtime).getTime();
      const currentTime = Date.now();

      let isNewFile = false;
      let isMovedFile = false;

      if (!fileIntenty) {
        // El archivo fue creado recientemente (dentro de los últimos 60 segundos)
        isNewFile = true;
      } else if (creationTime !== modificationTime) {
        // El archivo fue movido (o modificado)
        isMovedFile = true;
      }

      // Procesar el archivo según su estado
      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      )
        return;

      if (isNewFile) {
        this.fileInDevice.add(path);

        this.queueManager.enqueue({
          path,
          type: typeQueue.add,
          isFolder: false,
        });
      } else if (isMovedFile) {
        this.writeLog("File moved:", path);
        // Procesar archivo movido según sea necesario
      }
    } catch (error) {
      console.log("Error en onAdd");
      console.error(error);
    }
  };

  private onChange = (path: string, state: any) => {
    this.writeLog("onChange");
  };

  private onAddDir = (path: string, state: Stats) => {
    console.log("🚀 ~ Watcher ~ state:", state)
    console.log("🚀 ~ Watcher ~ path:", path)
    try {
      this.writeLog("onAddDir", path, state);
      const status: Status = this.virtualDriveFn.CfGetPlaceHolderState(path);
      this.writeLog("status", status);

      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      )
        return;

      this.queueManager.enqueue({
        path,
        type: typeQueue.add,
        isFolder: true,
      });
    } catch (error) {
      this.writeLog("Error en onAddDir");
      console.error(error);
    }
  };

  private onError = (error: any) => {
    this.writeLog("onError", error);
  };

  private onRaw = async (event: string, path: string, details: any) => {
    this.writeLog("onRaw", event, path, details);

    let isDirectory = false;

    if (event === "change" && details.prev && details.curr) {
      const item = await fs.statSync(path);
      if (item.isDirectory()) {
        this.writeLog("Es un directorio", path);
        isDirectory = true;
        return;
      }
      if (Path.extname(path) === "") {
        this.writeLog("Archivo sin extensión ignorado", path);
        return;
      }

      // // Ignorar archivos vacíos
      // if (item.size === 0) {
      //   this.writeLog("Archivo vacío ignorado", path);
      //   return;
      // }

      const action = await this.detectContextMenuAction(
        details,
        path,
        isDirectory
      );

      if (action) {
        this.writeLog(`Action detected: '${action}'`, path);
      }
    }
  };

  private async detectContextMenuAction(
    details: any,
    path: string,
    isDirectory: boolean
  ): Promise<string | null> {
    const { prev, curr } = details;
    const status = await this.virtualDriveFn.CfGetPlaceHolderState(path);
    this.writeLog("status", status);

    const attribute: Attributes =
      await this.virtualDriveFn.CfGetPlaceHolderAttributes(path);
    this.writeLog("attribute", attribute);

    const itemId = this.virtualDriveFn.CfGetPlaceHolderIdentity(path);
    this.writeLog("itemId", itemId);

    const isInDevice = this.fileInDevice.has(path);

    if (
      prev.size === curr.size &&
      prev.ctimeMs !== curr.ctimeMs &&
      prev.mtimeMs === curr.mtimeMs &&
      status.pinState === PinState.AlwaysLocal &&
      status.syncState === SyncState.InSync &&
      !isInDevice
    ) {
      this.fileInDevice.add(path);
      this.queueManager.enqueue({
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
      this.fileInDevice.delete(path);
      this.queueManager.enqueue({
        path,
        type: typeQueue.dehydrate,
        isFolder: isDirectory,
        fileId: itemId,
      });
      return "Liberar espacio";
    }

    if (prev.size != curr.size) {
      this.queueManager.enqueue({
        path,
        type: typeQueue.changeSize,
        isFolder: isDirectory,
        fileId: itemId,
      });
      this.fileInDevice.add(path);
      return "Cambio de tamaño";
    }

    return null;
  }

  private onReady = () => {
    this.writeLog("onReady");
  };

  public watchAndWait() {
    try {
      console.log("🚀 ~ Watcher ~ watchAndWait ~ this.syncRootPath:", this.syncRootPath)
      const watcher = chokidar.watch(this.syncRootPath, this.options);

      watcher
        .on("add", this.onAdd)
        .on("change", this.onChange)
        .on("addDir", this.onAddDir)
        .on("error", this.onError)
        .on("raw", this.onRaw)
        .on("ready", this.onReady);
    } catch (error) {
      this.writeLog("Error en watchAndWait");
      console.error(error);
    }
  }
}
