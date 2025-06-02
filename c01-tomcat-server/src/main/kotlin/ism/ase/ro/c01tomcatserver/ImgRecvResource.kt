package ism.ase.ro.c01tomcatserver

import com.fasterxml.jackson.annotation.JsonProperty
import jakarta.enterprise.context.ApplicationScoped
import jakarta.inject.Inject
import jakarta.ws.rs.Consumes
import jakarta.ws.rs.POST
import jakarta.ws.rs.Path
import jakarta.ws.rs.Produces
import jakarta.ws.rs.core.MediaType

data class InputJSON(
    @JsonProperty("imageName") val imageName: String,
    @JsonProperty("base64Image") val base64Image: String,
    @JsonProperty("operation") val operation: String,
    @JsonProperty("mode") val mode: String,
    @JsonProperty("key") val key: String,
)

data class OutputJSON(
    @JsonProperty("result") val result: String
)

@Path("/recv-img")
@ApplicationScoped
class ImgRecvResource {
    @Inject
    private lateinit var rabbitMQService: RabbitMQService

    @POST
    @Consumes(MediaType.APPLICATION_JSON)
    @Produces(MediaType.APPLICATION_JSON)
    fun receiveImage(input: InputJSON): OutputJSON {
        rabbitMQService.sendMessage(input.imageName, input.base64Image)
        return OutputJSON("Received image: ${input.imageName}")
    }
}