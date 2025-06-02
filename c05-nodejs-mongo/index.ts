import { MongoClient } from 'mongodb';

const MONGO_URL = "mongodb://c06:27017";
const client = new MongoClient(MONGO_URL, {
    directConnection: true,
    auth: {
        username: "root",
        password: "password"
    }
});

try {
    await client.connect();
} catch (error) {
    console.error("Failed to connect to MongoDB:", error);
    process.exit(1);
}

const db = client.db("p0");

Bun.serve({
    routes: {
        "/download-file/:userId/:fileName": async (req) => {
            const { userId, fileName } = req.params;
            if (!userId || !fileName) {
                return new Response("Invalid parameters", { status: 400 });
            }

            // Simulate file download logic
            const image = await db.collection("p0").findOne({
                userId: userId,
                imageName: fileName
            });
            if (!image || !image.imgBase64) {
                return new Response("File not found", { status: 404 });
            }

            const imgBuffer = Buffer.from(image.imgBase64, 'base64');
            let contentType = "application/octet-stream";
            if (image.imgBase64.startsWith("data:")) {
                const mimeMatch = image.imgBase64.match(/^data:([^;]+);base64,/);
                if (mimeMatch) {
                    contentType = mimeMatch[1];
                }
            }
            return new Response(imgBuffer, {
                headers: {
                    "Content-Type": contentType,
                    "Content-Disposition": `attachment; filename="${fileName}"`
                }
            });
        },
        "/post-image": {
            POST: async (req) => {
                const body: {
                    imgBase64: string;
                    imageName: string;
                    userId: string;
                    operation: string;
                    mode: string;
                } = await req.json() as any;

                if (!body.imgBase64 || !body.imageName || !body.userId || !body.operation || !body.mode) {
                    return new Response("Invalid request body", { status: 400 });
                }

                const exists = await db.collection("p0").findOne({
                    userId: body.userId,
                    imageName: body.imageName,
                    operation: body.operation,
                    mode: body.mode
                });
                if (exists) {
                    await db.collection("p0").updateOne({
                        _id: exists._id
                    }, {
                        $set: {
                            imgBase64: body.imgBase64
                        }
                    });
                } else {
                    await db.collection("p0").insertOne({
                        userId: body.userId,
                        imgBase64: body.imgBase64,
                        imageName: body.imageName,
                        operation: body.operation,
                        mode: body.mode,
                    });
                }
                return new Response("ok", { status: 200 });
            }
        }
    },
    fetch(req) {
        return new Response("Not Found", { status: 404 });
    }
});