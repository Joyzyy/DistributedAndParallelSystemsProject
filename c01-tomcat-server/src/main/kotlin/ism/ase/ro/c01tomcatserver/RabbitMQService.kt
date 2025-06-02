package ism.ase.ro.c01tomcatserver

import com.rabbitmq.client.Channel
import com.rabbitmq.client.ConfirmCallback
import com.rabbitmq.client.Connection
import com.rabbitmq.client.ConnectionFactory
import jakarta.annotation.PostConstruct
import jakarta.enterprise.context.ApplicationScoped
import jakarta.enterprise.context.Initialized
import jakarta.enterprise.event.Observes
import java.util.concurrent.ConcurrentNavigableMap
import java.util.concurrent.ConcurrentSkipListMap
import kotlin.system.exitProcess

@ApplicationScoped
@jakarta.ejb.Singleton
@jakarta.ejb.Startup
class RabbitMQService {
    private lateinit var connection: Connection
    private lateinit var channel: Channel
    private var outstandingConfirms: ConcurrentNavigableMap<Long?, String?> = ConcurrentSkipListMap<Long?, String?>()
    private var cleanOutstandingConfirms: ConfirmCallback = ConfirmCallback { sequenceNumber: Long, multiple: Boolean ->
        if (multiple) {
            val confirmed = outstandingConfirms.headMap(
                sequenceNumber, true
            )
            confirmed.clear()
        } else {
            outstandingConfirms.remove(sequenceNumber)
        }
    }

    companion object {
        var EXCHANGE_NAME = "pictures"
    }

    fun onStartup(@Observes @Initialized(ApplicationScoped::class) event: Object) {
        println("RabbitMQService startup observer called")
    }

    @PostConstruct
    fun init() {
        try {
            println("Initializing rabbitmq client connection...")
            val factory = ConnectionFactory()
            factory.host = "localhost"

            connection = factory.newConnection()
            channel = connection.createChannel()

            channel.exchangeDeclare(EXCHANGE_NAME, "topic")
        } catch (exception: Exception) {
            error(exception)
        }
    }

    fun sendMessage(imageName: String, imageBase64: String) {
        val imgNameWithoutExtension = imageName.substringBefore(".")
        val key = "encrypt.${imgNameWithoutExtension}"
        channel.basicPublish("pictures", key, null, imageBase64.toByteArray(charset("UTF-8")))
    }

    @jakarta.annotation.PreDestroy
    fun destroy() {
        try {
            println("Deleting exchange $EXCHANGE_NAME...")
            channel.exchangeDelete(EXCHANGE_NAME)
            println("Closing rabbitmq client connection...")
            channel.close()
            connection.close()
        } catch (exception: Exception) {
            error(exception)
        }
    }
}