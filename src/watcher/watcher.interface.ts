import * as chokidar from "chokidar";
import { IQueueManager } from "src/queue/queueManager";
import { Status } from "src/types/placeholder.type";

export interface IWatcher {
  set virtualDriveFunctions(IVirtualDriveFunctions: IVirtualDriveFunctions);
  // set notifyCallbacks(callbacks: InputSyncCallbacks & ExtraCallbacks);
  set queueManager(queueManager: IQueueManager);
  set syncRootPath(syncRootPath: string);
  set options(options: chokidar.WatchOptions);
}

/*

encolado:
- path 
- si viene directorio 
- path base 
- encolar todos los de la path base

Upload() => {
  - archivo agregado
  - validaciones
  - evaluacion de estado
  ..
  - CfAddItem()
  ..
  - notify finish
  - finish   
}

onAdd -> () => {
  - archivo agregado
  - archivo encolado
  - finalizacion
  -> Upload() de la cola
}

onAdd -> () => {
  Upload()
}
xxt.txt
onChange -> () => {
  - archivo modificado path = xxx.txt
  - archivo encolado
  - finalizacion
  - size update -> CfUpdateItem( path )
  - 
}

-----------------------------------------

*/

export interface IVirtualDriveFunctions {
  CfHydrate?: () => void;
  CfDehydrate?: () => void;
  CfAddItem: () => void;
  CfNotifyMessage?: () => void;
  CfUpdateSyncStatus?: () => void;
  UpdatePinState?: () => void;
  CfUpdateItem?: () => void;
  CfGetPlaceHolderState: (path: string) => Status;
  CfConverToPlaceholder: (path: string, fileIdentity: string) => void;
}
