#ifndef SERVER_H
#define SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

#define SSID        "Livebox6-2580"
#define PASSWORD    "B*t83re9%5o8^8itMNUoNQx@4Nu@*8SL"

// Declaración de funciones
httpd_handle_t start_webserver();  // Inicia el servidor web
esp_err_t hello_handler(httpd_req_t *req);  // Manejador del URI /hello
void startServer();  // Inicia WiFi y servidor web

#endif
