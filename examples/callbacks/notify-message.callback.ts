async function onMessageCallback(
    message: string, 
    action: string, 
    errorName: string, 
    callback: (response: boolean) => void) {
    try {
        console.log("[Example] Message received: ", message);
        console.log("[Example] Action: ", action);
        console.log("[Example] Error name: ", errorName);
        await callback(true);
    } catch (error) {
        callback(false);
    }
}

export default onMessageCallback;