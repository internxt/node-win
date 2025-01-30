import { execSync } from "child_process";
import { existsSync } from "fs";
import { mkdir, writeFile } from "fs/promises";
import { join } from "path";
import { TEST_FILES } from "test/utils/setup.helper.test";
import { v4 } from "uuid";
import { beforeEach } from "vitest";
import { mockDeep } from "vitest-mock-extended";
import { Logger } from "winston";

import { QueueManager } from "@/queue/queue-manager";
import { sleep } from "@/utils";

import { OnAddDirService } from "./events/on-add-dir.service";
import { OnAddService } from "./events/on-add.service";
import { OnAllService } from "./events/on-all.service";
import { OnRawService } from "./events/on-raw.service";
import { Watcher } from "./watcher";
import { IVirtualDriveFunctions } from "./watcher.interface";

describe("Watcher", () => {
  const virtualDriveFn = mockDeep<IVirtualDriveFunctions>();
  const queueManager = mockDeep<QueueManager>();
  const logger = mockDeep<Logger>();
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
    watcher.init(queueManager, syncRootPath, options, logger, virtualDriveFn);
    watcher.watchAndWait();
  };

  const getEvents = () => {
    return onAll.execute.mock.calls.map((call) => call[0].event);
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
      expect(getEvents()).toEqual(["addDir"]);
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
      expect(getEvents()).toEqual(["addDir", "add"]);
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
      execSync(`mkdir ${folder}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toEqual(["addDir", "addDir"]);
      expect(onAddDir.execute).toHaveBeenCalledWith(expect.objectContaining({ path: folder }));
    });

    it("When add a file, then emit one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, `${v4()}.txt`);
      await setupWatcher(syncRootPath);

      // Act
      execSync(`echo. 2>${file}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toEqual(["addDir", "add"]);
      expect(onAdd.execute).toHaveBeenCalledWith(expect.objectContaining({ path: file }));
    });

    it("When add a folder and a file inside, then emit one addDir and one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      const file = join(folder, `${v4()}.txt`);
      await setupWatcher(syncRootPath);

      // Act
      execSync(`mkdir ${folder}`);
      execSync(`echo. 2>${file}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toEqual(["addDir", "addDir", "add"]);
      expect(onAdd.execute).toHaveBeenCalledWith(expect.objectContaining({ path: file }));
      expect(onAddDir.execute).toHaveBeenCalledWith(expect.objectContaining({ path: folder }));
    });
  });

  describe("When rename items", () => {
    it("When rename a file, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const fileName1 = `${v4()}.txt`;
      const fileName2 = `${v4()}.txt`;
      const file1 = join(syncRootPath, fileName1);
      await setupWatcher(syncRootPath);
      await writeFile(file1, Buffer.alloc(1000));

      // Act
      execSync(`ren ${fileName1} ${fileName2}`, { cwd: syncRootPath });
      await sleep(50);

      // Assert
      expect(getEvents()).toEqual(["addDir", "add"]);
    });

    it("When rename a folder, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folderName1 = v4();
      const folderName2 = v4();
      const folder1 = join(syncRootPath, folderName1);
      await setupWatcher(syncRootPath);
      await mkdir(folder1);

      // Act
      execSync(`ren ${folderName1} ${folderName2}`, { cwd: syncRootPath });
      await sleep(50);

      // Assert
      expect(getEvents()).toEqual(["addDir", "addDir"]);
    });
  });

  describe("When move items", () => {
    it("When move a file to a folder, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      const file = join(syncRootPath, `${v4()}.txt`);
      await setupWatcher(syncRootPath);
      await mkdir(folder);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      execSync(`mv ${file} ${folder}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toEqual(["addDir", "addDir", "add"]);
    });

    it("When move a folder to a folder, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      const folderName = v4();
      const folder1 = join(syncRootPath, folderName);
      const folder2 = join(folder, folderName);
      await setupWatcher(syncRootPath);

      // Act
      await mkdir(folder);
      await mkdir(folder1);
      execSync(`mv ${folder1} ${folder2}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toEqual(["addDir", "addDir", "addDir"]);
    });
  });

  describe("When delete items", () => {
    it("When delete a file, then emit one unlink event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, `${v4()}.txt`);
      await setupWatcher(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      execSync(`rm ${file}`);
      await sleep(150);

      // Assert
      expect(getEvents()).toEqual(["addDir", "add", "unlink"]);
    });

    it("When delete a folder, then emit one unlinkDir event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);
      await mkdir(folder);

      // Act
      await sleep(50);
      execSync(`rmdir ${folder}`);
      await sleep(150);

      // Assert
      expect(getEvents()).toEqual(["addDir", "addDir", "unlinkDir"]);
    });
  });
});
