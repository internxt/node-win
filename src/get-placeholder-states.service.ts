import { readdir } from "fs/promises";
import { join } from "path";

import { isFileInDevice } from "./is-file-in-device.service";
import VirtualDrive from "./virtual-drive";

type TProps = {
  self: VirtualDrive;
  path: string;
};

export const getPlaceholderStates = async ({ self, path }: TProps) => {
  const files = await readdir(path, { withFileTypes: true });

  const promises = files.map(async (file) => {
    const filePath = join(path, file.name);

    if (file.isDirectory()) {
      return getPlaceholderStates({ self, path: filePath });
    } else {
      const status = self.getPlaceholderState(filePath);
      if (isFileInDevice(status)) {
        const id = self.getFileIdentity(filePath);
        self.watcher.fileInDevice.add(id);
      }
    }
  });

  await Promise.all(promises);
};
