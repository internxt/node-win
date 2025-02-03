import * as chokidar from "chokidar";
import { Stats } from "fs";
import { Logger } from "winston";

import { QueueManager } from "@/queue/queue-manager";
import { IQueueManager } from "@/queue/queueManager";

import { OnAddDirService } from "./events/on-add-dir.service";
import { OnAddService } from "./events/on-add.service";
import { OnAllService } from "./events/on-all.service";
import { OnRawService } from "./events/on-raw.service";
import { Addon } from "@/addon-wrapper";

export namespace Watcher {
  export type TOptions = chokidar.WatchOptions;
}

export class Watcher {
  syncRootPath!: string;
  options!: Watcher.TOptions;
  addon!: Addon;
  queueManager!: IQueueManager;
  logger!: Logger;
  fileInDevice = new Set<string>();

  constructor(
    private readonly onAll: OnAllService = new OnAllService(),
    private readonly onAdd: OnAddService = new OnAddService(),
    private readonly onAddDir: OnAddDirService = new OnAddDirService(),
    private readonly onRaw: OnRawService = new OnRawService(),
  ) {}

  init(
    queueManager: QueueManager,
    syncRootPath: string,
    options: chokidar.WatchOptions,
    logger: Logger,
    addon: Addon,
  ) {
    this.queueManager = queueManager;
    this.syncRootPath = syncRootPath;
    this.options = options;
    this.logger = logger;
    this.addon = addon;
  }

  private onChange = (path: string, stats?: Stats) => {
    this.logger.info({ fn: "onChange", path });
  };

  private onError = (error: Error) => {
    this.logger.error("onError", error);
  };

  private onReady = () => {
    this.logger.info({ fn: "onReady" });
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
      this.logger.error("watchAndWait", error);
    }
  }
}
