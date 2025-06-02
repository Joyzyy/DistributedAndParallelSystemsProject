import com.rabbitmq.client.CancelCallback
import com.rabbitmq.client.ConnectionFactory
import com.rabbitmq.client.DeliverCallback
import com.rabbitmq.client.Delivery
import okhttp3.MediaType
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody
import java.io.File
import java.util.Base64
import com.fasterxml.jackson.databind.ObjectMapper
import okhttp3.RequestBody.Companion.toRequestBody

val mapper = ObjectMapper()
val client = OkHttpClient()

fun postRequest(url: String, params: Map<String, String>): String {
    val jsonBody = mapper.writeValueAsString(params)
    val mediaType = "application/json; charset=utf-8".toMediaType()
    val body = jsonBody.toRequestBody(mediaType)

    val request = Request.Builder().url(url).post(body).build()

    client.newCall(request).execute().use { response ->
        if (!response.isSuccessful) throw Exception("Unexpected code $response")
        return response.body?.string() ?: ""
    }
}

fun main() {
    val EXCHANGE_NAME = "pictures"

    val factory = ConnectionFactory()
    factory.host = "c02"
    val connection = factory.newConnection()
    val channel = connection.createChannel()

    channel.exchangeDeclare(EXCHANGE_NAME, "topic")
    val queueName = channel.queueDeclare().queue

    channel.queueBind(queueName, EXCHANGE_NAME, "send.*.*.*.*")

    println(" [*] Waiting for messages. To exit press CTRL+C")

    val deliverCallback = DeliverCallback { _: String, delivery: Delivery ->
        val message = String(delivery.body, charset("UTF-8"))

        println(" [x] Received '${delivery.envelope.routingKey}'")

        val routingKeys = delivery.envelope.routingKey.split(".")
        if (routingKeys.size != 5) {
            println("Invalid routing key")
            return@DeliverCallback
        }

        val userId = routingKeys[1]
        val operation = routingKeys[2]
        val mode = routingKeys[3]
        val imageNameWithoutExtension = routingKeys[4]

        val key = message.substringBefore(";")
        println("Key: $key")
        val encryptedBytes = message.substringAfterLast(",")
        val bytes = Base64.getDecoder().decode(encryptedBytes)
        val fileNameToBeSaved = "${imageNameWithoutExtension}.bmp"
        File(fileNameToBeSaved).writeBytes(bytes)

        val encMode = when (mode) {
            "ECB" -> "aes-128-ecb"
            "CBC" -> "aes-128-cbc"
            else -> "aes-128-cbc"
        }

        try {
            val projectDir = File(System.getProperty("user.dir"))
            val executable = File(projectDir, "executable_mpi")

            val process = ProcessBuilder("mpirun", "-np", "2", "--host", "c03,c04", executable.absolutePath, fileNameToBeSaved, operation, encMode, key)
                .redirectErrorStream(true)
                .start()
            process.inputStream.bufferedReader().use{reader->
                reader.lines().forEach { line -> println(line) }
            }

            val exitCode = process.waitFor()
            println("Process finished with exit code: $exitCode")
            if (exitCode != 0) {
                channel.basicPublish(EXCHANGE_NAME, "recieve.$userId.$imageNameWithoutExtension", null, "Process finished with error".toByteArray())
            }

            if (exitCode == 0) {
                // read enc file from the fs
                try {
                    val processedFileNameOnFs = "${imageNameWithoutExtension}_output.bin"
                    val finalImageName = when (operation) {
                        "encrypt" -> processedFileNameOnFs
                        "decrypt" -> "${imageNameWithoutExtension}_outputdecrypted.bmp"
                        else -> "unknown.bin"
                    }
                    val savedFileFromFs = File(finalImageName).readBytes()
                    val encodedBase64Image = Base64.getEncoder().encodeToString(savedFileFromFs)
                    val requestBody = mapOf(
                        "userId" to userId,
                        "operation" to operation,
                        "mode" to mode,
                        "imageName" to finalImageName,
                        "imgBase64" to encodedBase64Image
                    )

                    val response = postRequest("http://c05:3000/post-image", requestBody)
                    channel.basicPublish(
                        EXCHANGE_NAME,
                        "recieve.$userId.$imageNameWithoutExtension",
                        null,
                        finalImageName.toByteArray()
                    )
                } catch (e: Exception) {
                    println("Error posting image: ${e.message}")
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
    val cancelCallback = CancelCallback { _ -> }
    channel.basicConsume(queueName, true, deliverCallback, cancelCallback)
}