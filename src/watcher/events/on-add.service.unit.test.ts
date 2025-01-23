import { Stats } from "fs";
import { mockDeep } from "vitest-mock-extended";

import { typeQueue } from "@/queue/queueManager";
import { PinState, SyncState } from "@/types/placeholder.type";

import { Watcher } from "../watcher";
import { OnAddService } from "./on-add.service";

describe("Watcher onAdd", () => {
  const watcher = mockDeep<Watcher>();
  const onAdd = new OnAddService();

  beforeEach(() => {
    vi.clearAllMocks();
  });

  it('Should enqueue an "add" task if the file is new', () => {
    // Arrange
    // @ts-ignore
    watcher.virtualDriveFn.CfGetPlaceHolderIdentity.mockReturnValue(null);
    watcher.virtualDriveFn.CfGetPlaceHolderState.mockReturnValue({
      pinState: PinState.Unspecified,
      syncState: SyncState.NotInSync,
    });

    const path = "C:\\test-drive\\folder\\newfile.txt";
    const stats = { size: 1024, birthtime: new Date(), mtime: new Date() };

    // Act
    onAdd.execute({ self: watcher, path, stats: stats as unknown as Stats });

    // Assert
    expect(watcher.virtualDriveFn.CfGetPlaceHolderIdentity).toHaveBeenCalledWith(path);
    expect(watcher.virtualDriveFn.CfGetPlaceHolderState).toHaveBeenCalledWith(path);
    expect(watcher.queueManager.enqueue).toHaveBeenCalledWith({ path, type: typeQueue.add, isFolder: false });
  });

  it("Should not enqueue if the file is already in AlwaysLocal and InSync states", () => {
    // Arrange
    watcher.virtualDriveFn.CfGetPlaceHolderIdentity.mockReturnValue("existing-file-id");
    watcher.virtualDriveFn.CfGetPlaceHolderState.mockReturnValue({
      pinState: PinState.AlwaysLocal,
      syncState: SyncState.InSync,
    });

    const path = "C:\\test-drive\\folder\\existingFile.txt";
    const stats = { size: 2048, birthtime: new Date(), mtime: new Date() };

    // Act
    onAdd.execute({ self: watcher, path, stats: stats as unknown as Stats });

    // Assert
    expect(watcher.queueManager.enqueue).not.toHaveBeenCalled();
  });
});
