import { HandleAction, HandleActions } from "src/queue/queueManager";

import { logger } from "@/logger";
import { sleep } from "@/utils";

import { IQueueManager, QueueItem } from "../index";

export type QueueHandler = {
  handleAdd: HandleAction;
  handleHydrate: HandleAction;
  handleDehydrate: HandleAction;
  handleChange?: HandleAction;
  handleChangeSize: HandleAction;
};

export class QueueManager implements IQueueManager {
  private _queue: QueueItem[] = [];

  private isProcessing = false;

  actions: HandleActions;

  constructor(handlers: QueueHandler) {
    this.actions = {
      add: handlers.handleAdd,
      hydrate: handlers.handleHydrate,
      dehydrate: handlers.handleDehydrate,
      change: handlers.handleChange || (() => Promise.resolve()),
      changeSize: handlers.handleChangeSize,
    };
  }

  public enqueue(task: QueueItem): void {
    logger.debug({ fn: "enqueue", task });
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
    const task = this._queue.shift();
    if (!task) return;

    logger.debug({ fn: "processNext", task });

    switch (task.type) {
      case "add":
        return await this.actions.add(task);
      case "hydrate":
        return await this.actions.hydrate(task);
      case "dehydrate":
        return await this.actions.dehydrate(task);
      case "change":
        return await this.actions.change(task);
      case "changeSize":
        return await this.actions.changeSize(task);
      default:
        console.debug("Unknown task type.");
        break;
    }
  }

  public async processAll(): Promise<void> {
    logger.debug({ fn: "processAll", queueLength: this._queue.length });

    this.isProcessing = true;
    while (this._queue.length > 0) {
      await this.processNext();
      await sleep(200);
    }

    this.isProcessing = false;
  }
}
