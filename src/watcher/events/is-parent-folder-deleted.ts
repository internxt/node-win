import { win32 } from "path";

import { Watcher } from "../watcher";

type TProps = {
  self: Watcher;
  path: string;
};

export function isParentFolderDeleted({ self, path }: TProps) {
  const parentPath = path.split(win32.sep).slice(0, -1).join(win32.sep);

  const isDeleted = self.deletedDirs.has(parentPath);

  return { isDeleted };
}

