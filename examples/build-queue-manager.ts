import { QueueItem, VirtualDrive } from "@/index";
import { QueueManager } from "./queueManager";

export const buildQueueManager = (drive: VirtualDrive) => {
  const handleAdd = async (task: QueueItem) => {
    try {
      console.log("[EXAMPLE] File added in callback: " + task.path);
      const itemId = Math.random().toString(36).substring(2, 7);
      await drive.convertToPlaceholder(task.path, itemId);
      await drive.updateSyncStatus(task.path, task.isFolder, true);
    } catch (error) {
      console.error(error);
    }
  };

  const handleDehydrate = async (task: QueueItem) => {
    try {
      console.log("[EXAMPLE] File dehydrated in callback: " + task.path);
      await drive.dehydrateFile(task.path);
    } catch (error) {
      console.error(error);
    }
  };

  const handleHydrate = async (task: QueueItem) => {
    try {
      console.log("[EXAMPLE] File hydrated in callback: " + task.path);
      await drive.hydrateFile(task.path);
    } catch (error) {
      console.error(error);
    }
  };

  const handleChangeSize = async (task: QueueItem) => {
    try {
      console.log("[EXAMPLE] File size changed in callback: " + task.path);
      const result = Math.random().toString(36).substring(2, 7);
      await drive.convertToPlaceholder(task.path, result);
      await drive.updateFileIdentity(task.path, result, false);
      await drive.updateSyncStatus(task.path, task.isFolder, true);
      // await drive.updateFileSize(task.path);
    } catch (error) {
      console.error(error);
    }
  };

  return new QueueManager({
    handleAdd,
    handleHydrate,
    handleDehydrate,
    handleChangeSize,
  });
};
