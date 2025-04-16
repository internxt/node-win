import { execSync } from "child_process";
import { existsSync } from "fs";
import { appendFile, mkdir, rename, rm, unlink, writeFile } from "fs/promises";
import { join } from "path";
import { TEST_FILES } from "test/utils/setup.helper.test";
import { v4 } from "uuid";
import { beforeEach } from "vitest";
import { mockDeep } from "vitest-mock-extended";

import { Addon } from "@/addon-wrapper";
import { TLogger } from "@/logger";
import { QueueManager } from "@/queue/queue-manager";
import { sleep } from "@/utils";

import { OnAddDirService } from "./events/on-add-dir.service";
import { OnAddService } from "./events/on-add.service";
import { OnRawService } from "./events/on-raw.service";
import { Watcher } from "./watcher";

describe("Watcher", () => {
  let watcher: Watcher | undefined;

  const addon = mockDeep<Addon>();
  const queueManager = mockDeep<QueueManager>();
  const logger = mockDeep<TLogger>();
  const options = {};

  const onAll = vi.fn();
  const onAdd = mockDeep<OnAddService>();
  const onAddDir = mockDeep<OnAddDirService>();
  const onRaw = mockDeep<OnRawService>();

  const setupWatcher = async (syncRootPath: string) => {
    if (!existsSync(syncRootPath)) {
      await mkdir(syncRootPath);
    }

    watcher = new Watcher(onAdd, onAddDir, onRaw);
    watcher.init(queueManager, syncRootPath, options, logger, addon);
    watcher.watchAndWait();
    watcher.chokidar?.on("all", (event, path) => onAll({ event, path }));
  };

  const getEvents = () => {
    return onAll.mock.calls.map((call) => ({ event: call[0].event, path: call[0].path }));
  };

  beforeEach(() => {
    vi.clearAllMocks();
  });

  afterEach(() => {
    watcher?.chokidar?.close();
  });

  describe("[Watcher] When call watchAndWait", () => {
    it("When folder is empty, then emit one addDir event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      await mkdir(syncRootPath);

      // Act
      await sleep(50);
      await setupWatcher(syncRootPath);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual([{ event: "addDir", path: syncRootPath }]);
    });

    it("When folder has one file, then emit one addDir and one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, v4());
      await mkdir(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      await setupWatcher(syncRootPath);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
        ]),
      );
    });
  });

  describe("[Watcher] When add items", () => {
    it("When add an empty folder, then emit one addDir event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);

      // Act
      await sleep(50);
      await mkdir(folder);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder },
        ]),
      );
    });

    it("When add a file, then emit one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);

      // Act
      await sleep(50);
      await writeFile(file, Buffer.alloc(1000));
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
        ]),
      );
    });

    it("When add a file of zero size, then emit one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);

      // Act
      await sleep(50);
      await writeFile(file, Buffer.alloc(0));
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
        ]),
      );
    });

    it("When add a folder and a file inside, then emit one addDir and one add event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      const file = join(folder, v4());
      await setupWatcher(syncRootPath);

      // Act
      await sleep(50);
      await mkdir(folder);
      await writeFile(file, Buffer.alloc(1000));
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder },
          { event: "add", path: file },
        ]),
      );
    });
  });

  describe("[Watcher] When modify items", () => {
    it("When modify a file, then emit one change event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const fileName = v4();
      const file = join(syncRootPath, fileName);
      await setupWatcher(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      await appendFile(file, Buffer.alloc(1000));
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
          { event: "change", path: file },
        ]),
      );
    });
  });

  describe("[Addon] When rename items", () => {
    it("When rename a file, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const fileName1 = v4();
      const fileName2 = v4();
      const file1 = join(syncRootPath, fileName1);
      const file2 = join(syncRootPath, fileName2);
      await setupWatcher(syncRootPath);
      await writeFile(file1, Buffer.alloc(1000));

      // Act
      await sleep(50);
      await rename(file1, file2);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file1 },
          { event: "add", path: file2 },
        ]),
      );
    });

    it("When rename a folder, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder1 = join(syncRootPath, v4());
      const folder2 = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);
      await mkdir(folder1);

      // Act
      await sleep(50);
      await rename(folder1, folder2);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder1 },
          { event: "unlinkDir", path: folder1 },
          { event: "addDir", path: folder2 },
        ]),
      );
    });
  });

  describe("[Addon] When move items", () => {
    it("When move a file to a folder, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      const fileName = v4();
      const file = join(syncRootPath, fileName);
      const movedFile = join(folder, fileName);
      await setupWatcher(syncRootPath);
      await mkdir(folder);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      await rename(file, movedFile);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
          { event: "addDir", path: folder },
          { event: "add", path: movedFile },
        ]),
      );
    });

    it("When move a folder to a folder, then emit one unlinkDir and one addDir event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      const folderName = v4();
      const folder1 = join(syncRootPath, folderName);
      const folder2 = join(folder, folderName);
      await setupWatcher(syncRootPath);
      await mkdir(folder);
      await mkdir(folder1);

      // Act
      await sleep(50);
      await rename(folder1, folder2);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder },
          { event: "addDir", path: folder1 },
          { event: "unlinkDir", path: folder1 },
          { event: "addDir", path: folder2 },
        ]),
      );
    });
  });

  describe("[Addon] When delete items", () => {
    it("When delete a file, then emit one unlink event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const file = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      await unlink(file);
      await sleep(150);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
          { event: "unlink", path: file },
        ]),
      );
    });

    it("When delete a folder, then emit one unlinkDir event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);
      await mkdir(folder);

      // Act
      await sleep(50);
      await rm(folder, { recursive: true, force: true });
      await sleep(150);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder },
          { event: "unlinkDir", path: folder },
        ]),
      );
    });
  });

  describe("[Watcher] When pin items", () => {
    it("When pin a file, then emit one change event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const fileName = v4();
      const file = join(syncRootPath, fileName);
      await setupWatcher(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      execSync(`attrib +P ${file}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
          { event: "change", path: file },
        ]),
      );
    });

    it("When pin a folder, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);
      await mkdir(folder);

      // Act
      await sleep(50);
      execSync(`attrib +P ${folder}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder },
        ]),
      );
    });
  });

  describe("[Watcher] When unpin items", () => {
    it("When unpin a file, then emit one change event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const fileName = v4();
      const file = join(syncRootPath, fileName);
      await setupWatcher(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      execSync(`attrib +P ${file}`);
      await sleep(50);
      execSync(`attrib -P ${file}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
          { event: "change", path: file },
          { event: "change", path: file },
        ]),
      );
    });

    it("When unpin a folder, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);
      await mkdir(folder);

      // Act
      await sleep(50);
      execSync(`attrib +P ${folder}`);
      await sleep(50);
      execSync(`attrib -P ${folder}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder },
        ]),
      );
    });
  });

  describe("[Watcher] When set items to online only", () => {
    it("When set a file to online only, then emit one change event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const fileName = v4();
      const file = join(syncRootPath, fileName);
      await setupWatcher(syncRootPath);
      await writeFile(file, Buffer.alloc(1000));

      // Act
      await sleep(50);
      execSync(`attrib -P +U ${file}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "add", path: file },
          { event: "change", path: file },
        ]),
      );
    });

    it("When set a folder to online only, then do not emit any event", async () => {
      // Arrange
      const syncRootPath = join(TEST_FILES, v4());
      const folder = join(syncRootPath, v4());
      await setupWatcher(syncRootPath);
      await mkdir(folder);

      // Act
      await sleep(50);
      execSync(`attrib -P +U ${folder}`);
      await sleep(50);

      // Assert
      expect(getEvents()).toStrictEqual(
        expect.arrayContaining([
          { event: "addDir", path: syncRootPath },
          { event: "addDir", path: folder },
        ]),
      );
    });
  });
});
