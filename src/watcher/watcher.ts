import * as chokidar from "chokidar";
import fs, { Stats } from "fs";
import { inspect } from "util";

import { IQueueManager } from "../queue/queueManager";
import { OnAddDirService } from "./events/on-add-dir.service";
import { OnAddService } from "./events/on-add.service";
import { OnAllService } from "./events/on-all.service";
import { OnRawService } from "./events/on-raw.service";
import { IVirtualDriveFunctions } from "./watcher.interface";

export namespace Watcher {
  export type TOptions = chokidar.WatchOptions;
}

export class Watcher {
  syncRootPath!: string;
  options!: Watcher.TOptions;
  virtualDriveFn!: IVirtualDriveFunctions;
  queueManager!: IQueueManager;
  logPath!: string;
  fileInDevice = new Set<string>();

  constructor(
    private readonly onAll: OnAllService = new OnAllService(),
    private readonly onAdd: OnAddService = new OnAddService(),
    private readonly onAddDir: OnAddDirService = new OnAddDirService(),
    private readonly onRaw: OnRawService = new OnRawService(),
  ) {}

  init(
    queueManager: IQueueManager,
    syncRootPath: string,
    options: chokidar.WatchOptions,
    logPath: string,
    virtualDriveFn: IVirtualDriveFunctions,
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

  private onChange = (path: string, stats?: Stats) => {
    this.writeLog("onChange", path, stats);
  };

  private onError = (error: Error) => {
    this.writeLog("onError", error);
  };

  private onReady = () => {
    this.writeLog("onReady");
  };

  public watchAndWait() {
    try {
      const watcher = chokidar.watch(this.syncRootPath, this.options);

      watcher
        .on("all", (event, path, stats) => this.onAll.execute({ self: this, event, path, stats }))
        .on("add", (path, stats) => this.onAdd.execute({ self: this, path, stats: stats! }))
        .on("change", this.onChange)
        .on("addDir", (path, stats) => this.onAddDir.execute({ self: this, path, stats: stats! }))
        .on("error", this.onError)
        .on("raw", (event, path, details) => this.onRaw.execute({ self: this, event, path, details }))
        .on("ready", this.onReady);
    } catch (error) {
      this.writeLog("Error en watchAndWait");
      console.error(error);
    }
  }
}
