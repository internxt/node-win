//@ts-ignore
import addon from '../../build/Release/addon';

import * as config from './config.json';

addon.unregisterSyncRoot(config.syncRootPath);