import { sleep } from "@/utils";

import { Watcher } from "../watcher";
import { isParentFolderDeleted } from "./is-parent-folder-deleted";

type TProps = {
  self: Watcher;
  path: string;
};

export async function onUnlink({ self, path }: TProps) {
  await sleep(2000);

  const { isDeleted } = isParentFolderDeleted({ self, path });

  if (!isDeleted) {
    void self.callbacks.handleDeleteFile({ path });
  }
}

