import * as chokidar from "chokidar";
import { IWatcher, IVirtualDriveFunctions } from "./watcher.interface";
import { PinState, Status, SyncState, Attributes } from "../types/placeholder.type";
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

  /**
   * Logs a message to both console and file.
   */
  public writeLog = (...messages: any[]) => {
    const logMessage = `${new Date().toISOString()} - ${messages
      .map((msg) => (typeof msg === "object" ? JSON.stringify(msg, null, 2) : msg))
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

  set virtualDriveFunctions(virtualDriveFn: IVirtualDriveFunctions) {
    this._virtualDriveFn = virtualDriveFn;
  }

  set queueManager(queueManager: IQueueManager) {
    this._queueManager = queueManager;
  }

  set syncRootPath(syncRootPath: string) {
    this._syncRootPath = syncRootPath;
  }

  set options(options: chokidar.WatchOptions) {
    this._options = options;
  }

  /**
   * Fired when a new file is added.
   */
  private onAdd = (path: string, stats: fs.Stats) => {
    try {
      this.writeLog("onAdd", path, stats);

      const ext = Path.extname(path);
      const { size, birthtime, mtime } = stats;
      const fileIdentity = this._virtualDriveFn.CfGetPlaceHolderIdentity(path);
      const status: Status = this._virtualDriveFn.CfGetPlaceHolderState(path);

      this.writeLog("fileIdentity in add", fileIdentity);
      this.writeLog("status", status);

      // Ignore files without an extension, empty, or larger than 20GB
      if (!ext || size === 0 || size > 20 * 1024 * 1024 * 1024) {
        return;
      }

      const creationTime = new Date(birthtime).getTime();
      const modificationTime = new Date(mtime).getTime();

      let isNewFile = false;
      let isMovedFile = false;

      // Determine if the file is new or moved
      if (!fileIdentity) {
        isNewFile = true;
      } else if (creationTime !== modificationTime) {
        isMovedFile = true;
      }

      // If already AlwaysLocal, OnlineOnly, or InSync, do nothing
      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      ) {
        return;
      }

      if (isNewFile) {
        this.fileInDevice.add(path);
        this._queueManager.enqueue({ path, type: typeQueue.add, isFolder: false });
      } else if (isMovedFile) {
        this.writeLog("File moved:", path);
        // Additional logic for moved files can be added here if needed
      }
    } catch (error) {
      this.writeLog("Error in onAdd", error);
    }
  };

  /**
   * Fired when an existing file changes.
   */
  private onChange = (path: string, stats: fs.Stats) => {
    this.writeLog("onChange", path, stats);
    // Additional logic for handling changes can be added if required
  };

  /**
   * Fired when a directory is added.
   */
  private onAddDir = (path: string, stats: fs.Stats) => {
    try {
      this.writeLog("onAddDir", path, stats);

      const status: Status = this._virtualDriveFn.CfGetPlaceHolderState(path);
      this.writeLog("status", status);

      if (
        status.pinState === PinState.AlwaysLocal ||
        status.pinState === PinState.OnlineOnly ||
        status.syncState === SyncState.InSync
      ) {
        return;
      }

      this._queueManager.enqueue({ path, type: typeQueue.add, isFolder: true });
    } catch (error) {
      this.writeLog("Error in onAddDir", error);
    }
  };

  /**
   * Error event from the watcher.
   */
  private onError = (error: any) => {
    this.writeLog("onError", error);
  };

  /**
   * Raw event handler that can capture low-level events.
   */
  private onRaw = async (event: string, path: string, details: any) => {
    this.writeLog("onRaw", event, path, details);

    if (event !== "change" || !details.prev || !details.curr) return;

    try {
      const item = fs.statSync(path);
      const isDirectory = item.isDirectory();

      if (isDirectory) {
        this.writeLog("It's a directory, ignoring raw change:", path);
        return;
      }

      // Ignore files without extension
      if (Path.extname(path) === "") {
        this.writeLog("File without extension ignored", path);
        return;
      }

      const action = await this.detectContextMenuAction(details, path, isDirectory);
      if (action) {
        this.writeLog(`Action detected: '${action}'`, path);
      }
    } catch (error) {
      this.writeLog("Error in onRaw", error);
    }
  };

  /**
   * Detects actions triggered by context menu operations (e.g., "Always keep on this device", "Free up space").
   */
  private async detectContextMenuAction(
    details: any,
    path: string,
    isDirectory: boolean
  ): Promise<string | null> {
    const { prev, curr } = details;
    const status = await this._virtualDriveFn.CfGetPlaceHolderState(path);
    const attribute: Attributes = await this._virtualDriveFn.CfGetPlaceHolderAttributes(path);
    const itemId = this._virtualDriveFn.CfGetPlaceHolderIdentity(path);
    const isInDevice = this.fileInDevice.has(path);

    this.writeLog("status", status);
    this.writeLog("attribute", attribute);
    this.writeLog("itemId", itemId);

    // Case: "Always keep on this device"
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
      return "Always keep on this device";
    }

    // Case: "Free up space"
    if (
      prev.size === curr.size &&
      prev.ctimeMs !== curr.ctimeMs &&
      status.pinState === PinState.OnlineOnly &&
      status.syncState === SyncState.InSync
    ) {
      this.fileInDevice.delete(path);
      this._queueManager.enqueue({
        path,
        type: typeQueue.dehydrate,
        isFolder: isDirectory,
        fileId: itemId,
      });
      return "Free up space";
    }

    // Case: Size change
    if (prev.size !== curr.size) {
      this.fileInDevice.add(path);
      this._queueManager.enqueue({
        path,
        type: typeQueue.changeSize,
        isFolder: isDirectory,
        fileId: itemId,
      });
      return "Size change";
    }

    return null;
  }

  /**
   * Fired when the watcher is ready.
   */
  private onReady = () => {
    this.writeLog("onReady");
  };

  /**
   * Starts watching the configured directory.
   */
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
      this.writeLog("Error in watchAndWait", error);
    }
  }
}
