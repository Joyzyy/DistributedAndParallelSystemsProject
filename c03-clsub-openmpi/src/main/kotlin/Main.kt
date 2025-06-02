package ism.ase.ro

import com.rabbitmq.client.CancelCallback
import com.rabbitmq.client.ConnectionFactory
import com.rabbitmq.client.DeliverCallback
import com.rabbitmq.client.Delivery

fun main() {
    val EXCHANGE_NAME = "pictures"

    val factory = ConnectionFactory()
    factory.host = "localhost"
    val connection = factory.newConnection()
    val channel = connection.createChannel()

    channel.exchangeDeclare(EXCHANGE_NAME, "topic")
    val queueName = channel.queueDeclare().queue

    channel.queueBind(queueName, EXCHANGE_NAME, "encrypt.*")

    println(" [*] Waiting for messages. To exit press CTRL+C")

    val deliverCallback = DeliverCallback { _: String, delivery: Delivery ->
        val message = String(delivery.body, charset("UTF-8"))
        println(" [x] Received '${delivery.envelope.routingKey}': '$message")
    }
    val cancelCallback = CancelCallback { _ -> }
    channel.basicConsume(queueName, true, deliverCallback, cancelCallback)
}