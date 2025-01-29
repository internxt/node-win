import settings from "examples/settings";
import { writeFileSync } from "fs";

writeFileSync(settings.syncRootPath, Buffer.alloc(1000));
