import { statSync } from "fs";
import { extname } from "path";

import { DetectContextMenuActionService } from "../detect-context-menu-action.service";
import { Watcher } from "../watcher";

export class OnRawService {
  constructor(private readonly detectContextMenuAction: DetectContextMenuActionService = new DetectContextMenuActionService()) {}

  async execute({ self, event, path, details }: TProps) {
    self.writeLog("onRaw", event, path, details);

    let isDirectory = false;

    if (event === "change" && details.prev && details.curr) {
      const item = statSync(path);

      if (item.isDirectory()) {
        self.writeLog("Es un directorio", path);
        isDirectory = true;
        return;
      }

      if (extname(path) === "") {
        self.writeLog("Archivo sin extensión ignorado", path);
        return;
      }

      // // Ignorar archivos vacíos
      // if (item.size === 0) {
      //   self.writeLog("Archivo vacío ignorado", path);
      //   return;
      // }

      const action = await this.detectContextMenuAction.execute({ self, details, path, isDirectory });

      if (action) {
        self.writeLog(`Action detected: '${action}'`, path);
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
