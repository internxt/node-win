type NapiCallbackFunction = (...args: unknown[]) => unknown;

type FilePlaceholderId = `FILE:${string}`;
type FolderPlaceholderId = `FOLDER:${string}`;
export type PlaceholderId = FilePlaceholderId | FolderPlaceholderId;

export type TFetchDataCallback = (
  id: FilePlaceholderId,
  callback: (
    data: boolean,
    path: string,
    errorHandler?: () => void,
  ) => Promise<{ finished: boolean; progress: number }>,
) => void;
export type TNotifyDeleteCallback = (id: PlaceholderId, callback: (response: boolean) => void) => void;

export type AddonCallbacks = {
  fetchDataCallback: TFetchDataCallback;
  validateDataCallback?: NapiCallbackFunction;
  cancelFetchDataCallback?: NapiCallbackFunction;
  fetchPlaceholdersCallback?: NapiCallbackFunction;
  cancelFetchPlaceholdersCallback?: NapiCallbackFunction;
  notifyFileOpenCompletionCallback?: NapiCallbackFunction;
  notifyFileCloseCompletionCallback?: NapiCallbackFunction;
  notifyDehydrateCallback?: NapiCallbackFunction;
  notifyDehydrateCompletionCallback?: NapiCallbackFunction;
  notifyDeleteCallback?: TNotifyDeleteCallback;
  notifyDeleteCompletionCallback?: NapiCallbackFunction;
  notifyRenameCallback?: NapiCallbackFunction;
  notifyRenameCompletionCallback?: NapiCallbackFunction;
  noneCallback?: NapiCallbackFunction;
};

export type VirtualDriveCallbacks = AddonCallbacks & {
  handleDeleteFile: ({ uuid }: { uuid: string }) => Promise<void>;
  handleDeleteFolder: ({ uuid }: { uuid: string }) => Promise<void>;
};
