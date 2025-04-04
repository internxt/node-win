import fs from "fs";
import { v4 } from "uuid";
import { Mock } from "vitest";
import { mockDeep } from "vitest-mock-extended";

import { addon } from "@/addon";

import { Callbacks } from "./types/callbacks.type";
import VirtualDrive from "./virtual-drive";

vi.mock("fs");
vi.mock("@/addon", () => ({
  addon: {
    addLoggerPath: vi.fn().mockReturnValue(true),
    connectSyncRoot: vi.fn(),
    createPlaceholderFile: vi.fn(),
    registerSyncRoot: vi.fn().mockReturnValue(0),
  },
}));

describe("VirtualDrive", () => {
  const mockExistsSync = fs.existsSync as Mock;

  const syncRootPath = "C:\\test-drive";
  const logPath = "C:\\test-logs";

  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe("When convertToWindowsPath is called", () => {
    const providerId = v4();

    // Arrange
    const drive = new VirtualDrive(syncRootPath, providerId, logPath);

    it("When unix path, then convert to windows path", () => {
      // Assert
      const result = drive.convertToWindowsPath("C:/test-drive/test.txt");
      expect(result).toBe("C:\\test-drive\\test.txt");
    });

    it("When windows path, then do not modify it", () => {
      // Assert
      const result = drive.convertToWindowsPath("C:\\test-drive\\test.txt");
      expect(result).toBe("C:\\test-drive\\test.txt");
    });
  });

  describe("When fixPath is called", () => {
    const providerId = v4();

    // Arrange
    const drive = new VirtualDrive(syncRootPath, providerId, logPath);

    it("When absolute windows path, then do not modify it", () => {
      // Assert
      expect(drive.fixPath("C:\\test-drive\\test.txt")).toBe("C:\\test-drive\\test.txt");
    });

    it("When absolute unix path, then convert to absolute windows path", () => {
      // Assert
      expect(drive.fixPath("C:/test-drive/test.txt")).toBe("C:\\test-drive\\test.txt");
    });

    it("When relative path, then convert to absolute windows path", () => {
      // Assert
      expect(drive.fixPath("test.txt")).toBe("C:\\test-drive\\test.txt");
    });

    it("When relative windows path, then convert to absolute windows path", () => {
      // Assert
      expect(drive.fixPath("\\test.txt")).toBe("C:\\test-drive\\test.txt");
    });

    it("When relative unix path, then convert to absolute windows path", () => {
      // Assert
      expect(drive.fixPath("/test.txt")).toBe("C:\\test-drive\\test.txt");
    });
  });

  describe("When VirtualDrive is created", () => {
    it("When syncRootPath does not exist, then it creates it", () => {
      // Arrange
      mockExistsSync.mockReturnValue(false);

      const providerId = v4();

      // Act
      new VirtualDrive(syncRootPath, providerId, logPath);

      // Assert
      expect(fs.mkdirSync).toHaveBeenCalledWith(syncRootPath, {
        recursive: true,
      });
    });

    it("When syncRootPath exists, then it doesn't create it", () => {
      // Arrange
      mockExistsSync.mockReturnValue(true);

      const providerId = v4();

      // Act
      new VirtualDrive(syncRootPath, providerId, logPath);

      // Assert
      expect(fs.mkdirSync).not.toHaveBeenCalled();
    });

    it("Then it calls addon.addLoggerPath with logPath provided", () => {
      // Act
      const providerId = v4();

      new VirtualDrive(syncRootPath, providerId, logPath);

      // Assert
      expect(addon.addLoggerPath).toHaveBeenCalledWith(logPath);
    });
  });

  describe("When call createFileByPath", () => {
    it("Then it calls addon.createPlaceholderFile", () => {
      // Arrange
      mockExistsSync.mockReturnValue(true);
      const providerId = v4();

      const drive = new VirtualDrive(syncRootPath, providerId, logPath);

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
      const providerId = v4();
      const drive = new VirtualDrive(syncRootPath, providerId, logPath);
      const providerName = "MyProvider";
      const providerVersion = "1.0.0";
      const logoPath = "C:\\iconPath";
      const callbacks = mockDeep<Callbacks>();

      // Act
      expect(drive.callbacks).toBe(undefined);
      await drive.registerSyncRoot(providerName, providerVersion, callbacks, logoPath);

      // Assert
      expect(drive.callbacks).not.toBe(undefined);
      expect(addon.registerSyncRoot).toHaveBeenCalledWith(syncRootPath, providerName, providerVersion, providerId, logoPath);
    });
  });
});
