package ism.ase.ro.c01tomcatserver

import jakarta.ws.rs.ApplicationPath
import jakarta.ws.rs.core.Application

@ApplicationPath("/api")
class App : Application() {}