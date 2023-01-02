import {fileURLToPath} from 'node:url';
import path from 'node:path';
import Fastify from "fastify";
import {fastifyStatic} from "@fastify/static";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(path.dirname(__filename), '../'); // ensure __dirname is the top directory level

const fastify = Fastify({logger: false});
fastify.register(fastifyStatic, {
    root: path.join(__dirname, 'public'),
});

fastify.get('/', async (req, res) => {
    return res.sendFile('index.html');
});

fastify.listen({host: '0.0.0.0', port: 80}, (err, address) => {
    if (err) {
        console.error(err);
        process.exit(1);
    }
    console.log(`Server is now listening on ${address}`);
});
