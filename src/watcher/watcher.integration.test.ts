import { QueueManager } from "examples/queueManager";
import { mkdir, writeFile } from "fs/promises";
import { TEST_FILES } from "test/utils/setup.helper.test";
import { v4 } from "uuid";
import { beforeEach } from "vitest";
import { mockDeep } from "vitest-mock-extended";

import { Watcher } from "./watcher";
import { IVirtualDriveFunctions } from "./watcher.interface";

describe("Watcher", () => {
  const virtualDriveFn = mockDeep<IVirtualDriveFunctions>();
  const queueManager = mockDeep<QueueManager>();
  const syncRootPath = `${TEST_FILES}/${v4()}`;
  const logPath = `${TEST_FILES}/${v4()}.log`;
  const options = {};
  console.log("🚀 ~ describe ~ syncRootPath:", syncRootPath);

  const watcher = new Watcher();
  watcher.init(queueManager, syncRootPath, options, logPath, virtualDriveFn);

  beforeEach(async () => {
    await mkdir(syncRootPath);
  });

  it("", async () => {
    // Arrange
    watcher.watchAndWait();

    // Act
    await writeFile(`${syncRootPath}/watcher-integration.txt`, Buffer.alloc(1000));
  });
});
