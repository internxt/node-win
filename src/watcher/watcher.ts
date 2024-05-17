import * as chokidar from "chokidar";
import { IWatcher, IVirtualDriveFunctions } from "./watcher.interface";
import { PinState, Status, SyncState } from "../types/placeholder.type";
import { IQueueManager, typeQueue } from "../queue/queueManager";
import fs from "fs";

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

  private writeLog = (...messages: any[]) => {
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

      const { size } = state;

      if (!ext || size === 0 || size > 20 * 1024 * 1024 * 1024) return;

      const status: Status = this._virtualDriveFn.CfGetPlaceHolderState(path);
      this.writeLog("status", status);

      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      )
        return;

      this.fileInDevice.add(path);

      this._queueManager.enqueue({
        path,
        type: typeQueue.add,
        isFolder: false,
      });
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
    this.writeLog("onRaw", event, path, details);

    let isDirectory = false;

    if (event === "change" && details.prev && details.curr) {
      const item = await fs.statSync(path);
      if (item.isDirectory()) {
        this.writeLog("Es un directorio", path);
        isDirectory = true;
        return;
      }
      const action = this.detectContextMenuAction(details, path, isDirectory);

      if (action) {
        this.writeLog(`Action detected: '${action}'`, path);
      }
    }
  };

  private detectContextMenuAction(
    details: any,
    path: string,
    isDirectory: boolean
  ): string | null {
    const { prev, curr } = details;
    const status = this._virtualDriveFn.CfGetPlaceHolderState(path);

    const itemId = this._virtualDriveFn.CfGetPlaceHolderIdentity(path);

    const isInDevice = this.fileInDevice.has(path);

    this.writeLog("status", status);
    if (
      prev.size === curr.size && // Tamaño no cambia
      prev.ctimeMs !== curr.ctimeMs && // ctime cambia
      prev.mtimeMs === curr.mtimeMs && // mtime no cambia
      status.pinState === PinState.AlwaysLocal && // Estado es AlwaysLocal
      status.syncState === SyncState.InSync && // Estado es InSync
      !isInDevice // No está en el dispositivo
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

    if (prev.size != curr.size) {
      this._queueManager.enqueue({
        path,
        type: typeQueue.changeSize,
        isFolder: isDirectory,
        fileId: itemId,
      });
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
