import com.fasterxml.jackson.annotation.JsonProperty
import com.rabbitmq.client.CancelCallback
import com.rabbitmq.client.Channel
import io.javalin.Javalin
import com.rabbitmq.client.ConnectionFactory
import com.rabbitmq.client.DeliverCallback
import com.rabbitmq.client.Delivery
import io.javalin.http.bodyAsClass
import io.javalin.websocket.WsContext
import kotlin.text.set

fun sendMessage(exchangeName: String,
                channel: Channel,
                userId: String,
                imageName: String,
                mode: String,
                operation: String,
                key: String,
                base64Image: String) {
    // split .jpg
    val imageWithoutExtenstion = imageName.split(".")[0]
    println(" [x] Sent '$imageWithoutExtenstion'")
    val routingKey = "send.$userId.$operation.$mode.$imageWithoutExtenstion"
    val message = "$key;$base64Image"
    channel.basicPublish(exchangeName, routingKey, null, message.toByteArray(charset("UTF-8")))
}

data class RecieveImageJsonBody(
    @JsonProperty("userId") val userId: String,
    @JsonProperty("imageName") val imageName: String,
    @JsonProperty("base64Image") val base64Image: String,
    @JsonProperty("operation") val operation: String,
    @JsonProperty("mode") val mode: String,
    @JsonProperty("key") val key: String
)

data class RecieveImageJsonResponse(
    @JsonProperty("status") val status: String,
)

data class WSMessage(
    @JsonProperty("action") val action: String,
    @JsonProperty("data") val data: String
)

fun main() {
    val EXCHANGE_NAME = "pictures"

    // userId, operation
    val wsContexts = mutableMapOf<String, WsContext>()
    val userPostsMap = mutableMapOf<String, MutableList<String>>()

    val factory = ConnectionFactory()
    factory.host = "c02"
    val connection = factory.newConnection()
    val channel = connection.createChannel()

    channel.exchangeDeclare(EXCHANGE_NAME, "topic")

    println("Exchange '$EXCHANGE_NAME' declared successfully.")

    val queueName = channel.queueDeclare().queue
    channel.queueBind(queueName, EXCHANGE_NAME, "recieve.*.*")

    val deliverCallback = DeliverCallback{ _: String, delivery: Delivery ->
        val message = String(delivery.body, charset("UTF-8"))
        val routingKey = delivery.envelope.routingKey

        val parts = routingKey.split(".")
        val userId = parts[1]

        userPostsMap.entries.forEach { entry ->
            if (userId in entry.key) {
                println("userid: $userId, message: $message")
                wsContexts[entry.key]?.send("success,$message")
            }
        }
    }

    val cancelCallback = CancelCallback { consumerTag: String ->  }

    channel.basicConsume(queueName, true, deliverCallback, cancelCallback)

    Javalin.create{config ->
        config.bundledPlugins.enableCors{ cors ->
            cors.addRule { it ->
                it.anyHost()
                it.exposeHeader("*")
            }
        }
        config.jetty.modifyWebSocketServletFactory { factory ->
            factory.idleTimeout = java.time.Duration.ofMinutes(30L)
        }
        config.http.maxRequestSize = 100_000_000L
    }.get("/") { ctx ->
        ctx.result("Javalin server is running!")
    }.post("/api/recv-img"){ ctx ->
        val body = ctx.bodyAsClass<RecieveImageJsonBody>()
        userPostsMap[body.userId] = mutableListOf("${body.operation}.${body.mode}.${body.imageName.split(".")[0]}")
        sendMessage(EXCHANGE_NAME,
            channel,
            body.userId,
            body.imageName,
            body.mode,
            body.operation,
            body.key,
            body.base64Image)
        ctx.json(RecieveImageJsonResponse("success"))
    }.ws("/ws"){ ws ->
        ws.onConnect { ctx ->
            println("connected: ${ctx.host()}")
        }
        ws.onClose { ctx ->
            println("closed: ${ctx.host()}")
            wsContexts.entries.removeIf { it.value == ctx }
            userPostsMap.entries.removeIf { it.value.remove(ctx.host()) }
        }
        ws.onMessage { ctx ->
            val body = ctx.messageAsClass<WSMessage>()
            if (body.action == "setUserId") {
                wsContexts[body.data] = ctx
            }
        }
    }.start("0.0.0.0", 7777)
}