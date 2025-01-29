import { mkdirSync } from "fs";
import { join } from "path";
import { cwd } from "process";

export const TMP_PATH = join(cwd(), "examples", "tmp");
mkdirSync(TMP_PATH, { recursive: true });

const settings = {
  driveName: "Internxt",
  driveVersion: "2.0.4",
  syncRootPath: join(TMP_PATH, "sync-root"),
  defaultLogPath: join(TMP_PATH, "drive.log"),
  defaultIconPath: join(cwd(), "assets", "icon.ico"),
  watcherLogPath: join(TMP_PATH, "watcher.log"),
};

export default settings;
