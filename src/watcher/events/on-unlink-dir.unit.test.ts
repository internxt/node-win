import { mkdir, rm, writeFile } from "fs/promises";
import { join } from "path";
import { TEST_FILES } from "test/utils/setup.helper.test";
import { testSleep } from "test/utils/utils.helper.test";
import { v4 } from "uuid";

import { sleep } from "@/utils";

import { setupWatcher, getEvents, watcher } from "../watcher.helper.test";

vi.mock(import("@/utils.js"));

describe("[Watcher] on-unlink-dir", () => {
  const sleepMock = vi.mocked(sleep);

  beforeEach(() => {
    sleepMock.mockImplementation((ms) => testSleep(ms / 10));
  });

  it("When delete a folder", async () => {
    // Arrange
    const syncRootPath = join(TEST_FILES, v4());
    const folder = join(syncRootPath, v4());
    await setupWatcher(syncRootPath);
    await mkdir(folder);

    // Act
    await testSleep(50);
    await rm(folder, { recursive: true, force: true });
    await testSleep(150);

    // Assert
    expect(getEvents()).toStrictEqual([
      { event: "addDir", path: syncRootPath },
      { event: "addDir", path: folder },
      { event: "unlinkDir", path: folder },
    ]);

    await testSleep(200);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([folder]));
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledTimes(1);
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledWith({ path: folder });
    expect(watcher?.callbacks.handleDeleteFile).toHaveBeenCalledTimes(0);

    await testSleep(300);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([]));
  });

  it("When delete a folder with a file inside", async () => {
    // Arrange
    const syncRootPath = join(TEST_FILES, v4());
    const folder = join(syncRootPath, v4());
    const file = join(folder, v4());
    await setupWatcher(syncRootPath);
    await mkdir(folder);
    await writeFile(file, Buffer.alloc(1000));

    // Act
    await testSleep(50);
    await rm(folder, { recursive: true, force: true });
    await testSleep(150);

    // Assert
    expect(getEvents()).toStrictEqual([
      { event: "addDir", path: syncRootPath },
      { event: "addDir", path: folder },
      { event: "add", path: file },
      { event: "unlinkDir", path: folder },
      { event: "unlink", path: file },
    ]);

    await testSleep(200);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([folder]));
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledTimes(1);
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledWith({ path: folder });
    expect(watcher?.callbacks.handleDeleteFile).toHaveBeenCalledTimes(0);

    await testSleep(300);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([]));
  });

  it("When delete a folder with a folder inside", async () => {
    // Arrange
    const syncRootPath = join(TEST_FILES, v4());
    const folder1 = join(syncRootPath, v4());
    const folder2 = join(folder1, v4());
    await setupWatcher(syncRootPath);
    await mkdir(folder1);
    await mkdir(folder2);

    // Act
    await testSleep(50);
    await rm(folder1, { recursive: true, force: true });
    await testSleep(150);

    // Assert
    expect(getEvents()).toStrictEqual([
      { event: "addDir", path: syncRootPath },
      { event: "addDir", path: folder1 },
      { event: "addDir", path: folder2 },
      { event: "unlinkDir", path: folder2 },
      { event: "unlinkDir", path: folder1 },
    ]);

    await testSleep(200);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([folder1, folder2]));
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledTimes(1);
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledWith({ path: folder1 });
    expect(watcher?.callbacks.handleDeleteFile).toHaveBeenCalledTimes(0);

    await testSleep(300);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([]));
  });

  it("When delete a folder with more deep", async () => {
    // Arrange
    const syncRootPath = join(TEST_FILES, v4());
    const folder1 = join(syncRootPath, v4());
    const folder2 = join(folder1, v4());
    const file1 = join(folder1, v4());
    const file2 = join(folder2, v4());
    await setupWatcher(syncRootPath);
    await mkdir(folder1);
    await mkdir(folder2);
    await writeFile(file1, Buffer.alloc(1000));
    await writeFile(file2, Buffer.alloc(1000));

    // Act
    await testSleep(50);
    await rm(folder1, { recursive: true, force: true });
    await testSleep(150);

    // Assert
    expect(getEvents()).toStrictEqual([
      { event: "addDir", path: syncRootPath },
      { event: "addDir", path: folder1 },
      { event: "add", path: file1 },
      { event: "addDir", path: folder2 },
      { event: "add", path: file2 },
      { event: "unlinkDir", path: folder2 },
      { event: "unlinkDir", path: folder1 },
      { event: "unlink", path: file1 },
      { event: "unlink", path: file2 },
    ]);

    await testSleep(200);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([folder1, folder2]));
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledTimes(1);
    /**
     * v2.5.3 Daniel Jiménez
     * If we delete a folder with items inside it will emit an event for each of them.
     * However, in the backend we only to set the root folder as trashed/deleted.
     * Using the deleteDirs map we achieve this so only the root folder is calling handleDeleteFolder.
     */
    expect(watcher?.callbacks.handleDeleteFolder).toHaveBeenCalledWith({ path: folder1 });
    expect(watcher?.callbacks.handleDeleteFile).toHaveBeenCalledTimes(0);

    await testSleep(300);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([]));
  });
});

