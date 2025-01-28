import { stat } from "fs/promises";
import { extname } from "path";

import { DetectContextMenuActionService } from "../detect-context-menu-action.service";
import { Watcher } from "../watcher";

export class OnRawService {
  constructor(private readonly detectContextMenuAction: DetectContextMenuActionService = new DetectContextMenuActionService()) {}

  async execute({ self, event, path, details }: TProps) {
    if (event === "change" && details.prev && details.curr) {
      if (extname(path) === "") {
        self.writeLog({ event: "onRaw", path, details: "No extension" });
        return;
      }

      const item = await stat(path);
      if (item.isDirectory()) {
        self.writeLog({ event: "onRaw", path, details: "Is directory" });
        return;
      }

      // // Ignorar archivos vacíos
      // if (item.size === 0) {
      //   self.writeLog("Archivo vacío ignorado", path);
      //   return;
      // }

      const action = await this.detectContextMenuAction.execute({ self, details, path, isFolder: false });

      if (action) {
        self.writeLog({ event: "onRaw", path, action });
      }
    }
  }
}

type TProps = {
  self: Watcher;
  event: string;
  path: string;
  details: any;
};
