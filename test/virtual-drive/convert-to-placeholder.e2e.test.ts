import VirtualDrive from "@/virtual-drive";
import settings from "examples/settings";
import { mockDeep } from "jest-mock-extended";
import { mkdir, writeFile } from "fs/promises";
import { join } from "path";
import { v4 } from "uuid";
import { PinState, SyncState } from "@/types/placeholder.type";
import { clearFolder } from "../utils/clear-folder.helper.test";
import { Callbacks } from "@/types/callbacks.type";

describe("Convert to placeholder", () => {
  const drive = new VirtualDrive(
    settings.syncRootPath,
    settings.defaultLogPath
  );

  const callbacks = mockDeep<Callbacks>();

  beforeAll(async () => {
    await clearFolder(settings.syncRootPath);

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

  it("When file/folder does not exist should not create placeholder", () => {
    // Arrange
    const id = v4();
    const path = join(settings.syncRootPath, id);

    // Act
    const isCreated = drive.convertToPlaceholder(path, id);
    const status = drive.getPlaceholderState(path);

    // Assert
    expect(isCreated).toBe(false);
    expect(status).toEqual({
      pinState: PinState.Unspecified,
      syncState: SyncState.Undefined,
    });
  });

  describe("Convert file to placeholder", () => {
    it("Creates the placeholder and sets the sync state to undefined", async () => {
      // Arrange
      const id = v4();
      const path = join(settings.syncRootPath, `${id}.txt`);
      await writeFile(path, Buffer.alloc(1000));

      // Act
      const isCreated = drive.convertToPlaceholder(path, id);
      const status = drive.getPlaceholderState(path);

      // Assert
      expect(isCreated).toBe(true);
      expect(status).toEqual({
        pinState: PinState.AlwaysLocal,
        syncState: SyncState.InSync,
      });
    });

    it("When trying to convert to placeholder two times it ignores the second time", async () => {
      // Arrange
      const id = v4();
      const path = join(settings.syncRootPath, `${id}.txt`);
      await writeFile(path, Buffer.alloc(1000));

      // Act
      const isCreated1 = drive.convertToPlaceholder(path, id);
      const isCreated2 = drive.convertToPlaceholder(path, id);
      const status = drive.getPlaceholderState(path);

      // Assert
      expect(isCreated1).toBe(true);
      expect(isCreated2).toBe(false);
      expect(status).toEqual({
        pinState: PinState.AlwaysLocal,
        syncState: SyncState.InSync,
      });
    });
  });

  describe("Convert folder to placeholder", () => {
    it("Creates the placeholder and sets the sync state to undefined", async () => {
      // Arrange
      const id = v4();
      const path = join(settings.syncRootPath, id);
      await mkdir(path);

      // Act
      const isCreated = drive.convertToPlaceholder(path, id);
      const status = drive.getPlaceholderState(path);

      // Assert
      expect(isCreated).toBe(true);
      expect(status).toEqual({
        pinState: PinState.Unspecified,
        syncState: SyncState.InSync,
      });
    });

    it("When trying to convert to placeholder two times it ignores the second time", async () => {
      // Arrange
      const id = v4();
      const path = join(settings.syncRootPath, id);
      await mkdir(path);

      // Act
      const isCreated1 = drive.convertToPlaceholder(path, id);
      const isCreated2 = drive.convertToPlaceholder(path, id);
      const status = drive.getPlaceholderState(path);

      // Assert
      expect(isCreated1).toBe(true);
      expect(isCreated2).toBe(false);
      expect(status).toEqual({
        pinState: PinState.Unspecified,
        syncState: SyncState.InSync,
      });
    });

    it("Creates the placeholders and sets the sync state just for the folder", async () => {
      // Arrange
      const id = v4();
      const folderPath = join(settings.syncRootPath, id);
      const filePath = join(folderPath, `${id}.txt`);
      await mkdir(folderPath);
      await writeFile(filePath, Buffer.alloc(1000));

      // Act
      const isCreated = drive.convertToPlaceholder(folderPath, id);
      const folderStatus = drive.getPlaceholderState(folderPath);
      const fileStatus = drive.getPlaceholderState(filePath);

      // Assert
      expect(isCreated).toBe(true);
      expect(folderStatus).toEqual({
        pinState: PinState.Unspecified,
        syncState: SyncState.InSync,
      });
      expect(fileStatus).toEqual({
        pinState: PinState.Unspecified,
        syncState: SyncState.Undefined,
      });
    });
  });
});
