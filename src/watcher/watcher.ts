import { watch, FSWatcher } from "chokidar";

import { Addon } from "@/addon-wrapper";
import { TLogger } from "@/logger";
import { QueueManager } from "@/queue/queue-manager";

import { OnAddDirService } from "./events/on-add-dir.service";
import { OnAddService } from "./events/on-add.service";
import { OnRawService } from "./events/on-raw.service";
import { onUnlink } from "./events/on-unlink";
import { onUnlinkDir } from "./events/on-unlink-dir";

export type TWatcherCallbacks = {
  handleDeleteFile: ({ path }: { path: string }) => Promise<void>;
  handleDeleteFolder: ({ path }: { path: string }) => Promise<void>;
};

export class Watcher {
  syncRootPath: string;
  addon: Addon;
  queueManager!: QueueManager;
  logger: TLogger;
  callbacks!: TWatcherCallbacks;
  fileInDevice = new Set<string>();
  deletedDirs = new Set<string>();
  chokidar?: FSWatcher;

  constructor(
    {
      addon,
      logger,
      queueManager,
      syncRootPath,
    }: {
      addon: Addon;
      logger: TLogger;
      queueManager: QueueManager;
      syncRootPath: string;
    },
    private readonly onAdd: OnAddService = new OnAddService(),
    private readonly onAddDir: OnAddDirService = new OnAddDirService(),
    private readonly onRaw: OnRawService = new OnRawService(),
  ) {
    this.addon = addon;
    this.logger = logger;
    this.queueManager = queueManager;
    this.syncRootPath = syncRootPath;
  }

  private onChange = (path: string) => {
    this.logger.debug({ msg: "onChange", path });
  };

  private onError = (error: Error) => {
    this.logger.error({ msg: "onError", error });
  };

  private onReady = () => {
    this.logger.debug({ msg: "onReady" });
  };

  watchAndWait({ callbacks }: { callbacks: TWatcherCallbacks }) {
    this.callbacks = callbacks;

    this.chokidar = watch(this.syncRootPath, {
      awaitWriteFinish: {
        stabilityThreshold: 2000,
        pollInterval: 100,
      },
      depth: undefined,
      followSymlinks: true,
      ignored: /(^|[/\\])\../,
      ignoreInitial: true,
      persistent: true,
      usePolling: true,
    });

    this.chokidar
      .on("add", (path, stats) => this.onAdd.execute({ self: this, path, stats: stats! }))
      .on("change", this.onChange)
      .on("addDir", (path, stats) => this.onAddDir.execute({ self: this, path, stats: stats! }))
      .on("unlinkDir", (path) => onUnlinkDir({ self: this, path }))
      .on("unlink", (path) => onUnlink({ self: this, path }))
      .on("error", this.onError)
      .on("raw", (event, path, details) => this.onRaw.execute({ self: this, event, path, details }))
      .on("ready", this.onReady);
  }
}
