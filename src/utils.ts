import * as fs from 'fs';
import * as path from 'path';

export function deleteAllSubfolders(directoryPath: string): void {
    // Comprobar si el directorio existe
    if (!fs.existsSync(directoryPath)) {
        console.error('El directorio especificado no existe:', directoryPath);
        return;
    }

    // Leer todos los elementos del directorio
    const items = fs.readdirSync(directoryPath);

    items.forEach(item => {
        let itemPath = path.join(directoryPath, item);
        let itemStats = fs.statSync(itemPath);

        // Si el elemento es una carpeta, eliminarla recursivamente
        if (itemStats.isDirectory() ) {
            deleteFolderRecursive(itemPath);
        }
        // Si el elemento es un archivo, eliminarlo
        if (itemStats.isFile()){
            fs.unlinkSync(itemPath);
        }
    });
}

export function deleteFolderRecursive(folderPath: string): void {
    if (fs.existsSync(folderPath)) {
        fs.readdirSync(folderPath).forEach((file, index) => {
            const currentPath = path.join(folderPath, file);
            if (fs.statSync(currentPath).isDirectory()) {
                deleteFolderRecursive(currentPath);
            } else {
                fs.unlinkSync(currentPath);
            }
        });

        // Después de eliminar todos los subdirectorios y archivos, eliminar la carpeta
        fs.rmdirSync(folderPath);
    }
}

// Uso de la función:
// deleteAllSubfolders('/ruta/del/directorio');
