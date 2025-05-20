import { unlink, writeFile } from "fs/promises";
import { join } from "path";
import { TEST_FILES } from "test/utils/setup.helper.test";
import { testSleep } from "test/utils/utils.helper.test";
import { v4 } from "uuid";

import { sleep } from "@/utils";

import { setupWatcher, getEvents, watcher } from "../watcher.helper.test";

vi.mock(import("@/utils.js"));

describe("[Watcher] on-unlink", () => {
  const sleepMock = vi.mocked(sleep);

  beforeEach(() => {
    sleepMock.mockImplementation((ms) => testSleep(ms / 10));
  });

  it("When delete a file, then emit one unlink event", async () => {
    // Arrange
    const syncRootPath = join(TEST_FILES, v4());
    const file = join(syncRootPath, v4());
    await setupWatcher(syncRootPath);
    await writeFile(file, Buffer.alloc(1000));

    // Act
    await testSleep(50);
    await unlink(file);
    await testSleep(150);

    // Assert
    expect(getEvents()).toStrictEqual([
      { event: "addDir", path: syncRootPath },
      { event: "add", path: file },
      { event: "unlink", path: file },
    ]);

    await testSleep(200);
    expect(watcher?.deletedDirs).toStrictEqual(new Set([]));
    expect(watcher?.callbacks.handleDeleteFile).toHaveBeenCalledTimes(1);
    expect(watcher?.callbacks.handleDeleteFile).toHaveBeenCalledWith({ path: file });
  });
});

