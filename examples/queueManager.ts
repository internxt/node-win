import { HandleAction, HandleActions } from "src/queue/queueManager";
import { IQueueManager, QueueItem } from "../index";

export type QueueHandler = {
  handleAdd: HandleAction;
  handleHidreate: HandleAction;
  handleDehidreate: HandleAction;
  handleChange?: HandleAction;
};
export async function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}
export class QueueManager implements IQueueManager {
  private _queue: QueueItem[] = [];

  private isProcessing = false;

  actions: HandleActions;

  constructor(handlers: QueueHandler) {
    this.actions = {
      add: handlers.handleAdd,
      hidreate: handlers.handleHidreate,
      dehidreate: handlers.handleDehidreate,
      change: handlers.handleChange || (() => Promise.resolve()),
    };
  }

  public enqueue(task: QueueItem): void {
    console.debug(`Task enqueued: ${JSON.stringify(task)}`);
    this._queue.push(task);
    this.sortQueue();
    if (!this.isProcessing) {
      this.processAll();
    }
  }

  private sortQueue(): void {
    this._queue.sort((a, b) => {
      if (a.isFolder && b.isFolder) {
        return 0;
      }
      if (a.isFolder) {
        return -1;
      }
      if (b.isFolder) {
        return 1;
      }
      return 0;
    });
  }

  public async processNext(): Promise<void> {
    if (this._queue.length === 0) {
      console.debug("No tasks in queue.");
      return;
    }
    const task = this._queue.shift();
    if (!task) return;
    console.debug(`Processing task: ${JSON.stringify(task)}`);
    switch (task.type) {
      case "add":
        return await this.actions.add(task);
      case "hidreate":
        return await this.actions.hidreate(task);
      case "dehidreate":
        return await this.actions.dehidreate(task);
      case "change":
        return await this.actions.change(task);
      default:
        console.debug("Unknown task type.");
        break;
    }
  }

  public async processAll(): Promise<void> {
    this.isProcessing = true;
    while (this._queue.length > 0) {
      await sleep(200);
      console.debug("Processing all tasks. Queue length:", this._queue.length);
      await this.processNext();
    }
    this.isProcessing = false;
  }
}
