import { Stats } from "fs";

import { Watcher } from "../watcher";

export class OnAllService {
  execute({ self, event, path, stats }: TProps) {}
}

type TProps = {
  self: Watcher;
  event: string;
  path: string;
  stats?: Stats;
};
