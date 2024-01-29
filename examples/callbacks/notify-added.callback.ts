function generateRandomNumber(min: number, max: number) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

async function onFileAddedCallback(filePath: string, callback: (aknowledge : boolean, id: string) => void) {

    try {
        let randomNumber = generateRandomNumber(10000, 60000);
        console.log("[EXAMPLE] File added in callback: " + filePath);
        await new Promise(resolve => setTimeout(() => {
            resolve(undefined);
        }, 1000));

        // primer argumento es el boolean que indica si se pudo crear el archivo o no en el cloud
        // segundo argumento es el id del archivo creado en el cloud
        const result = Math.random().toString(36).substring(2,7);
        callback(true, result); 
    } catch (error) {
        callback(false, '');
        console.error(error);
    }
}

export default onFileAddedCallback;