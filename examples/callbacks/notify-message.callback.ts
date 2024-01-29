async function onMessageCallback(
    message: string, 
    action: string, 
    errorName: string, 
    callback: (response: boolean) => void) {
    try {
        console.log("[EXAMPLE] Message received: ", message);
        console.log("[EXAMPLE] Action: ", action);
        console.log("[EXAMPLE] Error name: ", errorName);
        await callback(true);
    } catch (error) {
        callback(false);
    }
}

export default onMessageCallback;