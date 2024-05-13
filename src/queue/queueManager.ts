export enum typeQueue {
  add = "add",
  hidreate = "hidreate",
  dehidreate = "dehidreate",
  change = "change",
}

export type QueueItem = {
  path: string;
  isFolder: boolean;
  type: typeQueue;
  fileId?: string;
};

export type HandleAction = (task: QueueItem) => Promise<void>;

export type HandleActions = {
  [key in typeQueue]: HandleAction;
};

export interface IQueueManager {
  actions: HandleActions;

  enqueue(task: QueueItem): void;
}
