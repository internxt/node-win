import { execSync } from "child_process";
import { join } from "path";
import { v4 } from "uuid";

import settings from "./settings";

const rootFile1 = join(settings.syncRootPath, v4());
const rootFile2ChangeSize = join(settings.syncRootPath, `change-size-${v4()}.txt`);
const rootFile3 = join(settings.syncRootPath, `${v4()}.txt`);
const rootFile3Moved = join(settings.syncRootPath, `moved-${v4()}.txt`);
const rootFolder1 = join(settings.syncRootPath, v4());
const rootFolder2 = join(settings.syncRootPath, v4());
const folder1File1 = join(rootFolder1, `${v4()}.pdf`);
const folder1Folder1 = join(rootFolder1, v4());
const folder1Folder1File1 = join(folder1Folder1, `${v4()}.xlsx`);

execSync(`echo Hello, world! > ${rootFile1}`);
execSync(`echo Hello, world! > ${rootFile2ChangeSize}`);
execSync(`echo Hello, world! >> ${rootFile2ChangeSize}`);
execSync(`echo Hello, world! > ${rootFile3}`);
execSync(`mv ${rootFile3} ${rootFile3Moved}`);
execSync(`mkdir ${rootFolder1}`);
execSync(`mkdir ${rootFolder2}`);
execSync(`echo Hello, world! > ${folder1File1}`);
execSync(`mkdir ${folder1Folder1}`);
execSync(`echo Hello, world! > ${folder1Folder1File1}`);
