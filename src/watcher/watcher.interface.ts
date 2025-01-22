import * as chokidar from "chokidar";
import { IQueueManager } from "src/queue/queueManager";
import { Status } from "src/types/placeholder.type";

export interface IWatcher {
  set virtualDriveFunctions(IVirtualDriveFunctions: IVirtualDriveFunctions);
  set queueManager(queueManager: IQueueManager);
  set syncRootPath(syncRootPath: string);
  set options(options: chokidar.WatchOptions);
}

export interface IVirtualDriveFunctions {
  CfHydrate?: () => void;
  CfDehydrate?: () => void;
  CfAddItem: () => void;
  CfNotifyMessage?: () => void;
  CfUpdateSyncStatus: (
    path: string,
    sync: boolean,
    isDirectory: boolean
  ) => void;
  UpdatePinState?: () => void;
  CfUpdateItem?: () => void;
  CfGetPlaceHolderState: (path: string) => Status;
  CfGetPlaceHolderIdentity: (path: string) => string;
  CfGetPlaceHolderAttributes: (path: string) => Promise<any>;
  CfConverToPlaceholder: (path: string, fileIdentity: string) => void;
}
