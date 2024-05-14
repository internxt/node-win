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

  static get Instance() {
    if (!Watcher.instance) {
      Watcher.instance = new Watcher();
    }
    return Watcher.instance;
  }

  set virtualDriveFunctions(IVirtualDriveFunctions: IVirtualDriveFunctions) {
    this._virtualDriveFn = IVirtualDriveFunctions;
  }

  set queueManager(queueManager: IQueueManager) {
    this._queueManager = queueManager;
  }

  private onAdd = (path: string, state: any) => {
    try {
      console.log("onAdd", path, state);
      const ext = path.split(".").pop();

      const { size } = state;

      if (!ext || size === 0 || size > 20 * 1024 * 1024 * 1024) return;

      const status: Status = this._virtualDriveFn.CfGetPlaceHolderState(path);
      console.log("status", status);

      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      )
        return;

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
    console.log("onChange", path, state);
  };

  private onAddDir = (path: string, state: any) => {
    try {
      console.log("onAddDir", path, state);
      const status: Status = this._virtualDriveFn.CfGetPlaceHolderState(path);
      console.log("status", status);

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
      console.log("Error en onAddDir");
      console.error(error);
    }
  };

  private onError = (error: any) => {
    console.log("onError", error);
  };

  private onRaw = (event: string, path: string, details: any) => {
    console.log("onRaw", event, path, details);
    if (event === "change" && details.prev && details.curr) {
      fs.stat(path, (err, stats) => {
        if (err) {
          console.error(`Error getting stats for path: ${path}`, err);
          return;
        }
        if (stats.isDirectory()) {
          return;
        }
      });
      const action = this.detectContextMenuAction(details, path);

      if (action) {
        console.log(`Action detected: '${action}'`, path);
      }
    }
  };

  private detectContextMenuAction(details: any, path: string): string | null {
    const { prev, curr } = details;
    const status = this._virtualDriveFn.CfGetPlaceHolderState(path);

    console.log("status", status);
    if (
      prev.size === curr.size && // Tamaño no cambia
      prev.ctimeMs !== curr.ctimeMs && // ctime cambia
      prev.mtimeMs === curr.mtimeMs && // mtime no cambia
      status.pinState === PinState.AlwaysLocal // Estado es AlwaysLocal
    ) {
      this.queueManager.enqueue({
        path,
        type: typeQueue.hidreate,
        isFolder: false,
      });
      return "Mantener siempre en el dispositivo";
    }

    // Verificar si es "Liberar espacio"
    if (
      prev.size === curr.size && // Tamaño no cambia
      prev.ctimeMs !== curr.ctimeMs && // ctime cambia
      prev.mtimeMs === curr.mtimeMs && // mtime no cambia
      status.pinState === PinState.OnlineOnly // Estado es OnlineOnly
    ) {
      this.queueManager.enqueue({
        path,
        type: typeQueue.dehidreate,
        isFolder: false,
      });
      return "Liberar espacio";
    }

    return null;
  }

  private onReady = () => {
    console.log("onReady");
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
      console.log("Error en watchAndWait");
      console.error(error);
    }
  }
}
