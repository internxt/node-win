const addon = require('../build/Release/addon');

// Definición de constantes de atributo. Estos valores son ejemplos y pueden variar según la documentación de Windows.
const FILE_ATTRIBUTE_READONLY = 0x1;
const FILE_ATTRIBUTE_HIDDEN = 0x2;

// Combinando atributos para tener un archivo de solo lectura y oculto.
const combinedAttributes = null;
let creationTime = BigInt(Date.now()) * 10000n + 116444736000000000n;  // Representando el tiempo actual como ejemplo
let lastWriteTime = BigInt(Date.now()) * 10000n + 116444736000000000n;  // Un valor similar
let lastAccessTime = BigInt(Date.now()) * 10000n + 116444736000000000n;

console.log(addon.createPlaceholderFile(
    'prueba3.txt', 
    "742c2caa-eaae-4137-9515-173067c55045",
    123456, 
    combinedAttributes,
    creationTime.toString(), 
    lastWriteTime.toString(),
    lastAccessTime.toString(),
    "C:\\Users\\gcarl\\Desktop\\cloud2"
));
