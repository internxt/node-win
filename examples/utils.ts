import fs from 'fs';
import path from 'path';

interface FileDetail {
    path: string;
    size: number;
    baseDir: string;
}

function readFilesRecursively(dir: string, fileList: FileDetail[] = []): FileDetail[] {
    fs.readdirSync(dir).forEach(file => {
        const filePath = path.join(dir, file);
        if (fs.statSync(filePath).isDirectory()) {
            readFilesRecursively(filePath, fileList);
        } else {
            fileList.push({
                path: filePath,
                size: fs.statSync(filePath).size,
                baseDir: dir
            });
        }
    });
    return fileList;
}

function createFilesWithSize(sourceFolder: string, destFolder: string): void {
    const files: FileDetail[] = readFilesRecursively(sourceFolder);

    if (!fs.existsSync(destFolder)) {
        fs.mkdirSync(destFolder, { recursive: true });
    }
    
    files.forEach(file => {
        const relativePath = path.relative(file.baseDir, file.path);
        const destFilePath = path.join(file.baseDir.replace(sourceFolder, destFolder), relativePath);//path.join(destFolder, relativePath);
        const destFileDir = file.baseDir.replace(sourceFolder, destFolder);//path.dirname(destFilePath);

        if (!fs.existsSync(destFileDir)){
            fs.mkdirSync(destFileDir, { recursive: true });
        }

        fs.writeFileSync(destFilePath, Buffer.alloc(file.size));
    });

}

interface FilesInfo {
    [uuid: string]: string;
}
class ItemsInfoManager {
    private filePath: string;
    private data: FilesInfo;

    private constructor(filePath: string, data?: FilesInfo) {
        this.filePath = filePath;
        this.data = data || {};
    }

    public static async initialize(filePath?: string): Promise<ItemsInfoManager> {
        const resolvedPath = filePath ? path.resolve(filePath) : path.join(__dirname, 'filesInfo.json');
        let data: FilesInfo;

        try {
            const fileContent = await fs.promises.readFile(resolvedPath, 'utf8');
            data = JSON.parse(fileContent);
        } catch (error) {
            //@ts-ignore
            if (error.code === 'ENOENT') {
                data = {};
                await fs.promises.writeFile(resolvedPath, JSON.stringify(data));
            } else {
                console.error('Error initializing filesInfo.json:', error);
                throw error;
            }
        }

        return new ItemsInfoManager(resolvedPath, data);
    }

    public async add(info: FilesInfo): Promise<void> {
        try {
            this.data = { ...this.data, ...info };
            await fs.promises.writeFile(this.filePath, JSON.stringify(this.data, null, 2));
        } catch (error) {
            console.error('Error adding data to filesInfo.json:', error);
        }
    }

    public async remove(uuid: string): Promise<void> {
        try {
            if (this.data[uuid]) {
                delete this.data[uuid];
                await fs.promises.writeFile(this.filePath, JSON.stringify(this.data, null, 2));
            } else {
                console.log(`UUID ${uuid} not found.`);
            }
        } catch (error) {
            console.error('Error removing data from filesInfo.json:', error);
        }
    }

    public get(uuid: string): string | undefined {
        try {
            return this.data[uuid];
        } catch (error) {
            console.error('Error getting data from filesInfo.json:', error);
        }
    }
}

export { ItemsInfoManager, createFilesWithSize };