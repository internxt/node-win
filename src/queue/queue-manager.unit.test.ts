import { describe, it, expect, vi, beforeEach } from "vitest";

import { sleep } from "@/utils";

import { QueueHandler, QueueManager, QueueManagerCallback } from "./queue-manager";
import { QueueItem, typeQueue } from "./queueManager";

describe("QueueManager", () => {
  let queueManager: QueueManager;
  let mockHandlers: QueueHandler;
  let mockCallbacks: QueueManagerCallback;

  beforeEach(() => {
    mockHandlers = {
      handleAdd: vi.fn().mockResolvedValue(undefined),
      handleHydrate: vi.fn().mockResolvedValue(undefined),
      handleDehydrate: vi.fn().mockResolvedValue(undefined),
      handleChangeSize: vi.fn().mockResolvedValue(undefined),
    };

    mockCallbacks = {
      onTaskSuccess: vi.fn().mockResolvedValue(undefined),
      onTaskProcessing: vi.fn().mockResolvedValue(undefined),
    };

    queueManager = new QueueManager(mockHandlers, mockCallbacks, "mockPath.json");
  });

  it("should initialize the queue correctly", () => {
    expect(queueManager).toBeDefined();
  });

  it("should add a task to the queue and sort it correctly", async () => {
    const tasks: QueueItem[] = [
      { path: "\\test\\folder4", isFolder: true, type: typeQueue.add },
      { path: "\\test\\folder\\test.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test\\test.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test", isFolder: true, type: typeQueue.add },
      { path: "\\test\\test2.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test\\folder", isFolder: true, type: typeQueue.add },
      { path: "\\test\\folder2\\file-pdf", isFolder: false, type: typeQueue.add },
      { path: "\\test\\folder3", isFolder: true, type: typeQueue.add },
      { path: "\\test\\folder3\\file12.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test\\folder3\\folder3", isFolder: true, type: typeQueue.add },
    ];

    tasks.forEach((task) => queueManager.enqueue(task));
    await sleep(1000);

    console.log("queue", queueManager["queues"][typeQueue.add]);

    expect(queueManager["queues"][typeQueue.add]).toEqual([
      { path: "\\test", isFolder: true, type: typeQueue.add },
      { path: "\\test\\test.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test\\test2.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test\\folder", isFolder: true, type: typeQueue.add },
      { path: "\\test\\folder3", isFolder: true, type: typeQueue.add },
      { path: "\\test\\folder4", isFolder: true, type: typeQueue.add },
      { path: "\\test\\folder\\test.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test\\folder2\\file-pdf", isFolder: false, type: typeQueue.add },
      { path: "\\test\\folder3\\file12.txt", isFolder: false, type: typeQueue.add },
      { path: "\\test\\folder3\\folder3", isFolder: true, type: typeQueue.add },
    ]);
  });

  it("should not add a duplicate task", () => {
    const task: QueueItem = { path: "\\test.txt", isFolder: false, type: typeQueue.add };
    queueManager.enqueue(task);
    queueManager.enqueue(task);

    expect(queueManager["queues"][typeQueue.add].length).toBe(1);
  });

  it("should clear the queue", () => {
    queueManager.enqueue({ path: "\\test", isFolder: true, type: typeQueue.add });
    queueManager.clearQueue();
    expect(queueManager["queues"][typeQueue.add].length).toBe(0);
  });

  it("should correctly order deeply nested structures", () => {
    const tasks: QueueItem[] = [
      { path: "\\folder", isFolder: true, type: typeQueue.add },
      { path: "\\folder\\subfolder", isFolder: true, type: typeQueue.add },
      { path: "\\folder\\file.txt", isFolder: false, type: typeQueue.add },
      { path: "\\folder\\subfolder\\file2.txt", isFolder: false, type: typeQueue.add },
      { path: "\\folder\\subfolder\\deep", isFolder: true, type: typeQueue.add },
      { path: "\\folder\\subfolder\\deep\\file3.txt", isFolder: false, type: typeQueue.add },
    ];

    tasks.forEach((task) => queueManager.enqueue(task));

    expect(queueManager["queues"][typeQueue.add]).toEqual([
      { path: "\\folder", isFolder: true, type: typeQueue.add },
      { path: "\\folder\\file.txt", isFolder: false, type: typeQueue.add },
      { path: "\\folder\\subfolder", isFolder: true, type: typeQueue.add },
      { path: "\\folder\\subfolder\\file2.txt", isFolder: false, type: typeQueue.add },
      { path: "\\folder\\subfolder\\deep", isFolder: true, type: typeQueue.add },
      { path: "\\folder\\subfolder\\deep\\file3.txt", isFolder: false, type: typeQueue.add },
    ]);
  });

  it("should handle mixed folder\\file ordering properly", () => {
    const tasks: QueueItem[] = [
      { path: "\\alpha", isFolder: true, type: typeQueue.add },
      { path: "\\alpha\\file1.txt", isFolder: false, type: typeQueue.add },
      { path: "\\alpha\\file2.txt", isFolder: false, type: typeQueue.add },
      { path: "\\beta", isFolder: true, type: typeQueue.add },
      { path: "\\beta\\file3.txt", isFolder: false, type: typeQueue.add },
      { path: "\\gamma\\file4.txt", isFolder: false, type: typeQueue.add },
      { path: "\\gamma", isFolder: true, type: typeQueue.add },
    ];

    tasks.forEach((task) => queueManager.enqueue(task));

    expect(queueManager["queues"][typeQueue.add]).toEqual([
      { path: "\\alpha", isFolder: true, type: typeQueue.add },
      { path: "\\beta", isFolder: true, type: typeQueue.add },
      { path: "\\gamma", isFolder: true, type: typeQueue.add },
      { path: "\\alpha\\file1.txt", isFolder: false, type: typeQueue.add },
      { path: "\\alpha\\file2.txt", isFolder: false, type: typeQueue.add },
      { path: "\\beta\\file3.txt", isFolder: false, type: typeQueue.add },
      { path: "\\gamma\\file4.txt", isFolder: false, type: typeQueue.add },
    ]);
  });
});
