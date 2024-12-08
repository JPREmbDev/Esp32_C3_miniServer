#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "server.h"

static const char *TAG = "server";

// Mostrar la dirección IP después de la conexión Wi-Fi
void show_ip() {
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "Conectado a WiFi con IP: " IPSTR, IP2STR(&ip_info.ip));
    } else {
        ESP_LOGE(TAG, "Error obteniendo la dirección IP");
    }
}

// Manejador para responder "Hola Mundo" (GET)
esp_err_t hello_handler(httpd_req_t *req) {
    const char* resp_str = "Hola Mundo desde ESP32-C3!";
    ESP_LOGI(TAG, "Solicitud recibida en /hello, enviando respuesta...");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

// Manejador para recibir datos (POST)
esp_err_t message_handler(httpd_req_t *req) {
    char buf[100];  // Buffer para almacenar el mensaje recibido
    int ret, remaining = req->content_len;

    // Leer el contenido del cuerpo de la petición POST
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;  // Si el tiempo se agotó, intenta nuevamente
            }
            return ESP_FAIL;
        }
        remaining -= ret;
        buf[ret] = '\0';  // Añadir terminador de cadena al mensaje recibido
    }

    // Imprimir el mensaje recibido en los logs
    ESP_LOGI(TAG, "Mensaje recibido: %s", buf);

    // Enviar respuesta de confirmación al cliente
    const char* resp_str = "Mensaje recibido correctamente";
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}

// Configuración del servidor HTTP
httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Iniciando servidor web...");

    if (httpd_start(&server, &config) == ESP_OK) {
        // Registro de la URI /hello (GET)
        httpd_uri_t hello_uri = {
            .uri      = "/hello",
            .method   = HTTP_GET,
            .handler  = hello_handler,
            .user_ctx = NULL
        };

        ESP_LOGI(TAG, "Registrando URI /hello");
        httpd_register_uri_handler(server, &hello_uri);

        // Registro de la URI /message (POST)
        httpd_uri_t message_uri = {
            .uri      = "/message",
            .method   = HTTP_POST,
            .handler  = message_handler,
            .user_ctx = NULL
        };
        ESP_LOGI(TAG, "Registrando URI /message");
        httpd_register_uri_handler(server, &message_uri);

        ESP_LOGI(TAG, "Servidor iniciado en el puerto %d", config.server_port);
    } else {
        ESP_LOGE(TAG, "Error iniciando el servidor web");
    }

    return server;
}

// Inicializa WiFi y servidor web
// Declaración previa
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void startServer() {
    esp_err_t ret;

    // Inicializar NVS
    ESP_LOGI(TAG, "Inicializando NVS...");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Inicializar TCP/IP stack
    ESP_LOGI(TAG, "Inicializando TCP/IP stack...");
    ESP_ERROR_CHECK(esp_netif_init());

    // Crear el loop de eventos
    ESP_LOGI(TAG, "Inicializando bucle de eventos...");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Crear netif para estación WiFi
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Inicializar WiFi
    ESP_LOGI(TAG, "Inicializando WiFi...");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_LOGI(TAG, "Configurando manejadores de eventos...");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &event_handler,
                                                      NULL,
                                                      NULL));

    // Configure WiFi station
    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, "Livebox6-2580");
    strcpy((char *)wifi_config.sta.password, "B*t83re9%5o8^8itMNUoNQx@4Nu@*8SL");
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // Start WiFi
    ESP_LOGI(TAG, "Iniciando WiFi...");
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Inicialización completa. Intentando conectar a WiFi.");
}

// Manejador de eventos Wi-Fi
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    static bool server_started = false;
    
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi iniciando. Intentando conectar...");
                esp_wifi_connect();
                break;
                
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WiFi conectado, esperando IP...");
                break;
                
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WiFi desconectado, reintentando conexión...");
                esp_wifi_connect();
                if (server_started) {
                    server_started = false;
                }
                break;
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Obtenida IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Iniciar servidor web solo una vez que se obtiene la IP
        if (!server_started) {
            httpd_handle_t server = start_webserver();
            if (server != NULL) {
                server_started = true;
                ESP_LOGI(TAG, "Servidor web iniciado correctamente");
            }
        }
    }
}