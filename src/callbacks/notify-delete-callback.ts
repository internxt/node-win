import { PlaceholderId, TNotifyDeleteCallback } from "@/types/callbacks.type";
import type VirtualDrive from "@/virtual-drive";

type TProps = {
  self: VirtualDrive;
  id: PlaceholderId;
};

export const notifyDeleteCallback: TNotifyDeleteCallback = async ({ self, id }) => {
  self.deletedDirs.add(path);

  await sleep(2000);

  const { isDeleted } = isParentFolderDeleted({ self, path });

  if (!isDeleted) {
    void self.callbacks.handleDeleteFolder({ path });
  }

  await sleep(3000);

  self.deletedDirs.delete(path);
};

