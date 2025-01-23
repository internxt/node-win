import * as chokidar from "chokidar";
import { QueueManager } from "examples/queueManager";
import { mockDeep } from "vitest-mock-extended";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "./watcher";
import { IVirtualDriveFunctions } from "./watcher.interface";

vi.mock("fs");
vi.mock("chokidar", () => ({
  watch: vi.fn().mockReturnValue({
    on: vi.fn().mockReturnThis(),
  }),
}));

describe("Watcher", () => {
  const virtualDriveFn = mockDeep<IVirtualDriveFunctions>();
  const queueManager = mockDeep<QueueManager>();
  const syncRootPath = "C:\\test-drive";
  const logPath = "C:\\test-logs";
  const options = {};

  const watcher = new Watcher();
  watcher.init(queueManager, syncRootPath, options, logPath, virtualDriveFn);

  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe("When call watchAndWait", () => {
    it("Then configure chokidar with the correct path and options", () => {
      // Act
      watcher.watchAndWait();

      // Assert
      expect(chokidar.watch).toHaveBeenCalledWith(syncRootPath, options);
      const on = chokidar.watch("").on;
      expect(on).toHaveBeenCalledWith("add", expect.any(Function));
      expect(on).toHaveBeenCalledWith("change", expect.any(Function));
      expect(on).toHaveBeenCalledWith("addDir", expect.any(Function));
      expect(on).toHaveBeenCalledWith("error", expect.any(Function));
      expect(on).toHaveBeenCalledWith("raw", expect.any(Function));
      expect(on).toHaveBeenCalledWith("ready", expect.any(Function));
    });
  });

  describe("onAdd (simulated event)", () => {
    it('should enqueue an "add" task if the file is new', () => {
      //@ts-ignore
      virtualDriveFn.CfGetPlaceHolderIdentity.mockReturnValue(null);
      virtualDriveFn.CfGetPlaceHolderState.mockReturnValue({
        pinState: PinState.Unspecified,
        syncState: SyncState.NotInSync,
      });

      const path = "C:\\test-drive\\folder\\newfile.txt";
      const stats = {
        size: 1024,
        birthtime: new Date(),
        mtime: new Date(),
      };

      // Act
      (watcher as any).onAdd(path, stats);

      // Assert
      expect(virtualDriveFn.CfGetPlaceHolderIdentity).toHaveBeenCalledWith(path);
      expect(virtualDriveFn.CfGetPlaceHolderState).toHaveBeenCalledWith(path);
      expect(queueManager.enqueue).toHaveBeenCalledWith({ path, type: typeQueue.add, isFolder: false });
    });

    it("should not enqueue if the file is already in AlwaysLocal and InSync states", () => {
      virtualDriveFn.CfGetPlaceHolderIdentity.mockReturnValue("existing-file-id");
      virtualDriveFn.CfGetPlaceHolderState.mockReturnValue({
        pinState: PinState.AlwaysLocal,
        syncState: SyncState.InSync,
      });

      const path = "C:\\test-drive\\folder\\existingFile.txt";
      const stats = {
        size: 2048,
        birthtime: new Date(),
        mtime: new Date(),
      };

      // Act
      (watcher as any).onAdd(path, stats);

      // Assert
      expect(queueManager.enqueue).not.toHaveBeenCalled();
    });
  });
});
