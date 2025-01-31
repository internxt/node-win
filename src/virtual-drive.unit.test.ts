import fs from "fs";
import { Mock } from "vitest";

import { addon } from "@/addon";

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
    // Arrange
    const drive = new VirtualDrive(syncRootPath, logPath);

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

  describe("When fix path is called", () => {
    // Arrange
    const drive = new VirtualDrive(syncRootPath, logPath);

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

      // Act
      new VirtualDrive(syncRootPath, logPath);

      // Assert
      expect(fs.mkdirSync).toHaveBeenCalledWith(syncRootPath, { recursive: true });
    });

    it("When syncRootPath exists, then do not create it", () => {
      // Arrange
      mockExistsSync.mockReturnValue(true);

      // Act
      new VirtualDrive(syncRootPath, logPath);

      // Assert
      expect(fs.mkdirSync).not.toHaveBeenCalled();
    });

    it("Then call addon.addLoggerPath", () => {
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
});
