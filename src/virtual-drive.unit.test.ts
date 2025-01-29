import fs from "fs";
import { v4 } from "uuid";
import { Mock } from "vitest";

import { addon } from "@/addon";

import VirtualDrive from "./virtual-drive";

vi.mock("fs");
vi.mock("@/addon", () => ({
  addon: {
    addLoggerPath: vi.fn(),
    connectSyncRoot: vi.fn(),
    createPlaceholderFile: vi.fn(),
    registerSyncRoot: vi.fn(),
  },
}));

describe("VirtualDrive", () => {
  const mockExistsSync = fs.existsSync as Mock;

  const syncRootPath = "C:\\test-drive";
  const logPath = "C:\\test-logs";

  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe("When VirtualDrive is created", () => {
    it("When syncRootPath does not exist, then it creates it", () => {
      // Arrange
      mockExistsSync.mockReturnValue(false);

      // Act
      new VirtualDrive(syncRootPath, logPath);

      // Assert
      expect(fs.mkdirSync).toHaveBeenCalledWith(syncRootPath, {
        recursive: true,
      });
    });

    it("When syncRootPath exists, then it doesn't create it", () => {
      // Arrange
      mockExistsSync.mockReturnValue(true);

      // Act
      new VirtualDrive(syncRootPath, logPath);

      // Assert
      expect(fs.mkdirSync).not.toHaveBeenCalled();
    });

    it("Then it calls addon.addLoggerPath with logPath provided", () => {
      // Act
      new VirtualDrive(syncRootPath, logPath);

      // Assert
      expect(addon.addLoggerPath).toHaveBeenCalledWith(logPath);
    });
  });

  describe("When call createFileByPath", () => {
    it("Then it calls addon.createPlaceholderFile", () => {
      // Arrange
      mockExistsSync.mockReturnValue(true);
      const drive = new VirtualDrive(syncRootPath, logPath);

      // Act
      drive.createFileByPath("folder/subfolder/file.txt", "file-id", 1234, 1660000000000, 1660000001000);

      // Assert
      expect(addon.createPlaceholderFile).toHaveBeenCalledWith(
        "file.txt",
        "file-id",
        1234,
        1,
        expect.any(String),
        expect.any(String),
        expect.any(String),
        expect.stringContaining("C:\\test-drive\\folder\\subfolder"),
      );
    });
  });

  describe("When call registerSyncRoot", () => {
    it("Then it assigns callbacks and calls addon.registerSyncRoot", async () => {
      // Arrange
      const drive = new VirtualDrive(syncRootPath, logPath);
      const providerName = "MyProvider";
      const providerVersion = "1.0.0";
      const providerId = v4();
      const logoPath = "C:\\iconPath";
      const callbacks = {};

      // Act
      await drive.registerSyncRoot(providerName, providerVersion, providerId, callbacks, logoPath);

      // Assert
      expect(drive.callbacks).toBe(callbacks);
      expect(addon.registerSyncRoot).toHaveBeenCalledWith(syncRootPath, providerName, providerVersion, providerId, logoPath);
    });
  });
});
