import * as fs from 'fs';
import * as path from 'path';

type Options = { filtered: boolean, filterPaths: string[] };


export function deleteAllSubfolders(directoryPath: string, options?: Options): void {
    // Comprobar si el directorio existe
    if (!fs.existsSync(directoryPath)) {
        console.error('[Debug] El directorio especificado no existe:', directoryPath);
        return;
    }
    const rootPath = directoryPath;
    // Leer todos los elementos del directorio
    if (options?.filtered){
        deleteFolderRecursive(directoryPath, rootPath, options);
    } else {
        deleteFolderRecursive(directoryPath, rootPath);
    }
}

export function deleteFolderRecursive(folderPath: string, rootPath: string,options?: Options): void {
    if (fs.existsSync(folderPath)) {
        
        fs.readdirSync(folderPath).forEach((file, index) => {
            const currentPath = path.join(folderPath, file);
            if (fs.statSync(currentPath).isDirectory()) {
                options?.filtered ? deleteFolderRecursive(currentPath, rootPath, options) : deleteFolderRecursive(currentPath, rootPath);
            } else if (options?.filtered) {
                options?.filterPaths.includes(currentPath) ? fs.unlinkSync(currentPath): console.log('[Debug] Filtered: File is not in cloud storage so they are not deleted');
            } else {
                fs.unlinkSync(currentPath)
            }
        });
        
        // Después de eliminar todos archivos, eliminar la carpeta si no es la raiz
        if (options?.filtered && folderPath !== rootPath) {
            options.filterPaths.includes(folderPath) ? fs.rmdirSync(folderPath) : console.log('[Debug] Filtered: Folder is not in cloud storage so they are not deleted');
        } else if (folderPath !== rootPath) {
            fs.rmdirSync(folderPath);
        } 
        
    }
}
