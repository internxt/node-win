import { QueueManager } from "examples/queueManager";
import { existsSync } from "fs";
import { mkdir, writeFile } from "fs/promises";
import { join } from "path";
import { TEST_FILES } from "test/utils/setup.helper.test";
import { sleep } from "test/utils/sleep.helper.test";
import { v4 } from "uuid";
import { beforeEach } from "vitest";
import { mockDeep } from "vitest-mock-extended";

import { OnAddDirService } from "./events/on-add-dir.service";
import { OnAddService } from "./events/on-add.service";
import { OnAllService } from "./events/on-all.service";
import { OnRawService } from "./events/on-raw.service";
import { Watcher } from "./watcher";
import { IVirtualDriveFunctions } from "./watcher.interface";

describe("Watcher", () => {
  const virtualDriveFn = mockDeep<IVirtualDriveFunctions>();
  const queueManager = mockDeep<QueueManager>();
  const logPath = join(TEST_FILES, `${v4()}.log`);
  const options = {};

  const onAll = mockDeep<OnAllService>();
  const onAdd = mockDeep<OnAddService>();
  const onAddDir = mockDeep<OnAddDirService>();
  const onRaw = mockDeep<OnRawService>();

  const setupWatcher = async (syncRootPath: string) => {
    if (!existsSync(syncRootPath)) {
      await mkdir(syncRootPath);
    }

    const watcher = new Watcher(onAll, onAdd, onAddDir, onRaw);
    watcher.init(queueManager, syncRootPath, options, logPath, virtualDriveFn);
    watcher.watchAndWait();
  };

  beforeEach(async () => {
    vi.clearAllMocks();
  });

  describe("When call watchAndWait", () => {
    it("When folder is empty, then emit one addDir event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      await mkdir(syncRootPath);

      // Act
      await setupWatcher(syncRootPath);
      await sleep(50);

      // Assert
      expect(onAll.execute).toHaveBeenCalledTimes(1);
      expect(onAddDir.execute).toHaveBeenCalledWith(expect.objectContaining({ path: syncRootPath }));
    });

    it("When folder has one file, then emit one addDir and one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, `${v4()}.txt`);
      await mkdir(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await setupWatcher(syncRootPath);
      await sleep(50);

      // Assert
      expect(onAll.execute).toHaveBeenCalledTimes(2);
      expect(onAdd.execute).toHaveBeenCalledWith(expect.objectContaining({ path: file }));
      expect(onAddDir.execute).toHaveBeenCalledWith(expect.objectContaining({ path: syncRootPath }));
    });
  });

  describe("When add items", () => {
    it("When add an empty folder, then emit one addDir event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);

      // Act
      await mkdir(folder);
      await sleep(50);

      // Assert
      expect(onAll.execute).toHaveBeenCalledTimes(2);
      expect(onAddDir.execute).toHaveBeenCalledWith(expect.objectContaining({ path: folder }));
    });

    it("When add a file, then emit one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, `${v4()}.txt`);
      await setupWatcher(syncRootPath);

      // Act
      await writeFile(file, Buffer.alloc(1000));
      await sleep(50);

      // Assert
      expect(onAll.execute).toHaveBeenCalledTimes(2);
      expect(onAdd.execute).toHaveBeenCalledWith(expect.objectContaining({ path: file }));
    });

    it("When add a folder and a file inside, then emit one addDir and one add events", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      const file = join(folder, `${v4()}.txt`);
      await setupWatcher(syncRootPath);

      // Act
      await mkdir(folder);
      await writeFile(file, Buffer.alloc(1000));
      await sleep(50);

      // Assert
      expect(onAll.execute).toHaveBeenCalledTimes(3);
      expect(onAdd.execute).toHaveBeenCalledWith(expect.objectContaining({ path: file }));
      expect(onAddDir.execute).toHaveBeenCalledWith(expect.objectContaining({ path: folder }));
    });
  });
});
