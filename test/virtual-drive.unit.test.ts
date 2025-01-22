import VirtualDrive, { Callbacks } from "@/virtual-drive";
import fs from "fs";
import { addon } from "@/addon";
import { Mock } from "vitest";

vi.mock("fs");
vi.mock("@/addon", () => ({
  addon: {
    addLoggerPath: vi.fn(),
    connectSyncRoot: vi.fn(),
    createPlaceholderFile: vi.fn(),
    registerSyncRoot: vi.fn(),
  },
}));

const mockExistsSync = fs.existsSync as Mock;

describe("VirtualDrive Tests", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe("Constructor and syncRootPath creation", () => {
    it("should create the syncRootPath folder if it does not exist", () => {
      mockExistsSync.mockReturnValue(false);

      new VirtualDrive("C:\\test-drive");

      expect(fs.mkdirSync).toHaveBeenCalledWith("C:\\test-drive", {
        recursive: true,
      });
    });

    it("should not create the folder if it already exists", () => {
      mockExistsSync.mockReturnValue(true);

      new VirtualDrive("C:\\test-drive");

      expect(fs.mkdirSync).not.toHaveBeenCalled();
    });
  });

  describe("addLoggerPath", () => {
    it("should call addon.addLoggerPath with the log path", () => {
      mockExistsSync.mockReturnValue(true);

      new VirtualDrive("C:\\test-drive", "C:\\mis-logs");
      expect(addon.addLoggerPath).toHaveBeenCalledWith("C:\\mis-logs");
    });
  });

  describe("createFileByPath", () => {
    it("should invoke createPlaceholderFile with the correct arguments", () => {
      mockExistsSync.mockReturnValue(true);

      const drive = new VirtualDrive("C:\\test-drive");
      drive.createFileByPath(
        "folder/subfolder/file.txt",
        "file-id",
        1234,
        1660000000000,
        1660000001000
      );

      expect(addon.createPlaceholderFile).toHaveBeenCalledWith(
        "file.txt",
        "file-id",
        1234,
        1,
        expect.any(String),
        expect.any(String),
        expect.any(String),
        expect.stringContaining("C:\\test-drive\\folder\\subfolder")
      );
    });
  });

  describe("registerSyncRoot", () => {
    it("should call addon.registerSyncRoot and assign callbacks", async () => {
      const drive = new VirtualDrive("C:\\test-drive");
      const myCallbacks: Callbacks = {
        notifyDeleteCallback: vi.fn(),
        notifyRenameCallback: vi.fn(),
      };

      await drive.registerSyncRoot(
        "MyProvider",
        "1.0.0",
        "provider-id",
        myCallbacks,
        "C:\\iconPath"
      );

      expect(addon.registerSyncRoot).toHaveBeenCalledWith(
        "C:\\test-drive",
        "MyProvider",
        "1.0.0",
        "provider-id",
        "C:\\iconPath"
      );

      expect(drive.callbacks).toBe(myCallbacks);
    });
  });

  describe("connectSyncRoot", () => {
    it("should invoke addon.connectSyncRoot with the input callbacks", async () => {
      const drive = new VirtualDrive("C:\\test-drive");
      const myCallbacks: Callbacks = {
        fetchDataCallback: vi.fn(),
      };
      await drive.registerSyncRoot(
        "MyProvider",
        "1.0.0",
        "provider-id",
        myCallbacks,
        "C:\\iconPath"
      );

      await drive.connectSyncRoot();

      expect(addon.connectSyncRoot).toHaveBeenCalledWith(
        "C:\\test-drive",
        drive.getInputSyncCallbacks()
      );
    });
  });
});
