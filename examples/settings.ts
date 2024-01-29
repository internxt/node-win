import 'dotenv/config';
import Joi from 'joi';

// Definir el esquema de Joi para la validación
const envVarsSchema = Joi.object({
    EXAMPLE_DRIVE_NAME: Joi.string().required(),
    EXAMPLE_DRIVE_VERSION: Joi.string().required(),
    EXAMPLE_SYNC_ROOT_PATH: Joi.string().required(),
    EXAMPLE_DEFAULT_LOG_PATH: Joi.string().required(),
    EXAMPLE_DEFAULT_ICON_PATH: Joi.string().required()
}).unknown();

// Validar las variables de entorno
const { value: envVars, error } = envVarsSchema.validate(process.env);

if (error) {
    throw new Error(`Error de configuración: ${error.message}`);
}

// Exportar las variables validadas
const settings = {
    driveName: envVars.EXAMPLE_DRIVE_NAME,
    driveVersion: envVars.EXAMPLE_DRIVE_VERSION,
    syncRootPath: envVars.EXAMPLE_SYNC_ROOT_PATH,
    defaultLogPath: envVars.EXAMPLE_DEFAULT_LOG_PATH,
    defaultIconPath: envVars.EXAMPLE_DEFAULT_ICON_PATH
};

export default settings;
