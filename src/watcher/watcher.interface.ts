import * as chokidar from "chokidar";

export interface IWatcher {
  //   onAdd(path: string, state: any): void;

  //   onChange(path: string, state: any): void;

  //   onUnlink(path: string, state: any): void;

  //   onAddDir(path: string, state: any): void;

  //   onUnlinkDir(path: string): void;

  //   onError(error: any): void;

  //   onRaw(event: string, path: string, details: any): void;

  //   onReady(): void;

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
}
