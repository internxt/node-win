import { existsSync } from "fs";
import { mkdir } from "fs/promises";
import { mockDeep } from "vitest-mock-extended";

import { Addon } from "@/addon-wrapper";
import { TLogger } from "@/logger";
import { QueueManager } from "@/queue/queue-manager";

import { OnAddDirService } from "./events/on-add-dir.service";
import { OnAddService } from "./events/on-add.service";
import { OnRawService } from "./events/on-raw.service";
import { TWatcherCallbacks, Watcher } from "./watcher";

export let watcher: Watcher | undefined;

const addon = mockDeep<Addon>();
const queueManager = mockDeep<QueueManager>();
const logger = mockDeep<TLogger>();
const callbacks = mockDeep<TWatcherCallbacks>();

const onAll = vi.fn();
const onAdd = mockDeep<OnAddService>();
const onAddDir = mockDeep<OnAddDirService>();
const onRaw = mockDeep<OnRawService>();

export async function setupWatcher(syncRootPath: string) {
  if (!existsSync(syncRootPath)) {
    await mkdir(syncRootPath);
  }

  watcher = new Watcher({ queueManager, syncRootPath, logger, addon }, onAdd, onAddDir, onRaw);
  watcher.watchAndWait({ callbacks });
  watcher.chokidar?.on("all", (event, path) => onAll({ event, path }));
}

export function getEvents() {
  return onAll.mock.calls.map((call) => ({ event: call[0].event, path: call[0].path }));
}

beforeEach(() => {
  vi.clearAllMocks();
});

afterEach(async () => {
  await watcher?.chokidar?.close();
});

