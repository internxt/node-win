import { generateRandomFilesAndFolders } from '../../examples/utils/generate-random-file-tree';
import { VirtualDrive } from 'src';

class MockVirtualDrive implements Partial<VirtualDrive> {
  private files: Record<string, any> = {};
  private folders: Record<string, any> = {};

  createFileByPath(path: string, id: string, size: number, createdAt: number, updatedAt: number): void {
    this.files[path] = { id, size, createdAt, updatedAt };
  }

  createFolderByPath(path: string, id: string, size: number, createdAt: number, updatedAt: number): void {
    this.folders[path] = { id, size, createdAt, updatedAt };
  }

  getFiles(): Record<string, any> {
    return this.files;
  }

  getFolders(): Record<string, any> {
    return this.folders;
  }
}

describe('generateRandomFilesAndFolders', () => {
  it('should generate the correct structure of files and folders with sizes following a normal distribution', async () => {
    const mockDrive = new MockVirtualDrive();
    const meanSizeMB = 1; // 1 MB
    const stdDevMB = 0.5; // 0.5 MB
    const options = {
      rootPath: '/root',
      depth: 2,
      filesPerFolder: 10, 
      foldersPerLevel: 2,
      meanSize: meanSizeMB,
      stdDev: stdDevMB,
      timeOffset: 1000,
    };

    const result = await generateRandomFilesAndFolders(mockDrive as unknown as VirtualDrive, options);

    const files = mockDrive.getFiles();
    expect(Object.keys(files).length).toBeGreaterThan(0);

    const sizes = Object.values(files).map(file => file.size / (1024 * 1024)); 

    const lowerBound = meanSizeMB - 3 * stdDevMB;
    const upperBound = meanSizeMB + 3 * stdDevMB;

    const withinRange = sizes.filter(size => size >= lowerBound && size <= upperBound);
    const percentageWithinRange = (withinRange.length / sizes.length) * 100;

    expect(percentageWithinRange).toBeGreaterThanOrEqual(99);
  });
});
