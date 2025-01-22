import VirtualDrive, { Callbacks } from "@/virtual-drive";
import settings from "examples/settings";
import { mockDeep } from "jest-mock-extended";
import { generateFile } from "./utils/generate-file.helper.test";
import { writeFile } from "fs/promises";
import { buildQueueManager } from "examples/build-queue-manager";

describe("Virtual Drive + Watcher", () => {
  const drive = new VirtualDrive(
    settings.syncRootPath,
    settings.defaultLogPath
  );

  const queueManager = buildQueueManager(drive);
  const callbacks = mockDeep<Callbacks>();

  beforeAll(async () => {
    const providerId = "{12345678-1234-1234-1234-123456789012}";
    await drive.registerSyncRoot(
      settings.driveName,
      settings.driveVersion,
      providerId,
      callbacks,
      settings.defaultIconPath
    );

    await drive.connectSyncRoot();
  });

  afterAll(async () => {
    // await drive.disconnectSyncRoot();
    // VirtualDrive.unregisterSyncRoot(settings.syncRootPath);
  });

  describe("Add files and folders", () => {
    it("Add file", async () => {
      // Arrange
      const { fileId, fullPath, fileSize, createdAt, updatedAt } =
        generateFile();

      // Act
      drive.createFileByPath(fullPath, fileId, fileSize, createdAt, updatedAt);

      // Assert
      // const result = await drive.getPlaceholderAttribute(fullPath);
      // console.log("🚀 ~ it.only ~ result:", result);
    });
  });

  describe("Add files and folders manually", () => {
    it("Add file", async () => {
      // Arrange
      const { fullPath, fileSize } = generateFile();

      // Act
      await writeFile(fullPath, Buffer.alloc(fileSize));

      // Assert
      // const result = await drive.getPlaceholderAttribute(fullPath);
      // console.log("🚀 ~ it.only ~ result:", result);
      // drive.watchAndWait(
      //   settings.syncRootPath,
      //   queueManager,
      //   settings.watcherLogPath
      // );
    });
  });
});
