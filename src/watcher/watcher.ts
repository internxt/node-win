import * as chokidar from "chokidar";
import { IWatcher, IVirtualDriveFunctions } from "./watcher.interface";
import {
  PinState,
  Status,
  SyncState,
  Attributes,
} from "../types/placeholder.type";
import { IQueueManager, typeQueue } from "../queue/queueManager";
import fs from "fs";
import * as Path from "path";

export class Watcher implements IWatcher {
  private static instance: Watcher;

  private _syncRootPath: string = "";

  private _options!: chokidar.WatchOptions;

  private _virtualDriveFn!: IVirtualDriveFunctions;

  private _queueManager!: IQueueManager;

  private fileInDevice = new Set<string>();

  private _logPath!: string;

  static get Instance() {
    if (!Watcher.instance) {
      Watcher.instance = new Watcher();
    }
    return Watcher.instance;
  }

  public writeLog = (...messages: any[]) => {
    const logMessage = `${new Date().toISOString()} - ${messages
      .map((msg) =>
        typeof msg === "object" ? JSON.stringify(msg, null, 2) : msg
      )
      .join(" ")}\n`;
    console.log(logMessage);
    fs.appendFile(this._logPath, logMessage, (err) => {
      if (err) {
        console.error("Error writing to log file", err);
      }
    });
  };

  set logPath(logPath: string) {
    this._logPath = logPath;
  }

  set virtualDriveFunctions(IVirtualDriveFunctions: IVirtualDriveFunctions) {
    this._virtualDriveFn = IVirtualDriveFunctions;
  }

  set queueManager(queueManager: IQueueManager) {
    this._queueManager = queueManager;
  }

  private onAdd = (path: string, state: any) => {
    try {
      this.writeLog("onAdd", path, state);
      const ext = path.split(".").pop();

      const { size, birthtime, mtime } = state;

      const fileIntenty = this._virtualDriveFn.CfGetPlaceHolderIdentity(path);

      this.writeLog("fileIntenty in add", fileIntenty);

      if (!ext || size === 0 || size > 20 * 1024 * 1024 * 1024) return;

      const status: Status = this._virtualDriveFn.CfGetPlaceHolderState(path);
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

        this._queueManager.enqueue({
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

  private onAddDir = (path: string, state: any) => {
    try {
      this.writeLog("onAddDir", path, state);
      const status: Status = this._virtualDriveFn.CfGetPlaceHolderState(path);
      this.writeLog("status", status);

      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      )
        return;

      this._queueManager.enqueue({
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
    try {
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
    } catch (error) {
      this.writeLog("Error en onRaw");
      console.error(error);
    }
  };

  private async detectContextMenuAction(
    details: any,
    path: string,
    isDirectory: boolean
  ): Promise<string | null> {
    const { prev, curr } = details;
    const status = await this._virtualDriveFn.CfGetPlaceHolderState(path);
    this.writeLog("status", status);

    // const attribute: Attributes =
    //   await this._virtualDriveFn.CfGetPlaceHolderAttributes(path);
    // this.writeLog("attribute", attribute);

    const itemId = this._virtualDriveFn.CfGetPlaceHolderIdentity(path);
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
      this._queueManager.enqueue({
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
      this._queueManager.enqueue({
        path,
        type: typeQueue.dehydrate,
        isFolder: isDirectory,
        fileId: itemId,
      });
      return "Liberar espacio";
    }

    if (prev.size !== curr.size) {
      this._queueManager.enqueue({
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

  set syncRootPath(syncRootPath: string) {
    this._syncRootPath = syncRootPath;
  }

  set options(options: chokidar.WatchOptions) {
    this._options = options;
  }

  public watchAndWait() {
    try {
      const watcher = chokidar.watch(this._syncRootPath, this._options);

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
