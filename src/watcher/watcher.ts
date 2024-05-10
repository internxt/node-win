import * as chokidar from "chokidar";
import { IWatcher, IVirtualDriveFunctions } from "./watcher.interface";

export class Watcher implements IWatcher {
  private static instance: Watcher;

  private _syncRootPath: string = "";

  private _options!: chokidar.WatchOptions;

  private _virtualDriveFn!: IVirtualDriveFunctions;

  static get Instance() {
    if (!Watcher.instance) {
      Watcher.instance = new Watcher();
    }
    return Watcher.instance;
  }

  set virtualDriveFunctions(IVirtualDriveFunctions: IVirtualDriveFunctions) {
    this._virtualDriveFn = IVirtualDriveFunctions;
  }

  private onAdd = (path: string, state: any) => {
    console.log("onAdd", path, state);
    this._virtualDriveFn.CfAddItem();
  };
  /*
    onAdd -> () => {
    - archivo agregado
    - archivo encolado
    - finalizacion
    -> Upload() de la cola
  }

  Upload() => {
    - archivo agregado 
    - validaciones:  (que tenga extencion, que tenga zide, etc)
    - evaluacion de estado:  this.virualdrive.getState(path)
    ..
    - CfAddItem()
    ..
    - notify finish
    - finish   
  }
  */

  private onChange = (path: string, state: any) => {
    console.log("onChange", path, state);
    // this.virtualDriveFn.CfUpdateItem();
  };
  /*
    onChange -> () => {
    - archivo modificado path = xxx.txt
    - archivo encolado
    - finalizacion
    - size update -> CfUpdateItem( path )
    - 
  }
  */

  private onUnlink = (path: string, state: any) => {
    console.log("onUnlink", path, state);
  };

  private onAddDir = (path: string, state: any) => {
    console.log("onAddDir", path, state);
  };

  private onUnlinkDir = (path: string) => {
    console.log("onUnlinkDir", path);
  };

  private onError = (error: any) => {
    console.log("onError", error);
  };

  private onRaw = (event: string, path: string, details: any) => {
    console.log("onRaw", event, path, details);
  };

  private onReady = () => {
    console.log("onReady");
  };

  set syncRootPath(syncRootPath: string) {
    this._syncRootPath = syncRootPath;
  }

  set options(options: chokidar.WatchOptions) {
    this._options = options;
  }

  public watchAndWait() {
    try {
      const watcher = chokidar.watch(this._syncRootPath, this._options);

      watcher
        .on("add", this.onAdd)
        .on("change", this.onChange)
        .on("unlink", this.onUnlink)
        .on("addDir", this.onAddDir)
        .on("unlinkDir", this.onUnlinkDir)
        .on("error", this.onError)
        .on("raw", this.onRaw)
        .on("ready", this.onReady);
    } catch (error) {
      console.log("Error en watchAndWait");
      console.error(error);
    }
  }
}
