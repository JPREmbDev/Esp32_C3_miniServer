#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "server.h"
#include "esp_log.h"

// Tag para los logs de la aplicación
static const char *TAG = "main";

extern "C" void app_main() {
    // Establecemos el nivel de logs a INFO
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Iniciando aplicación en ESP32-C3...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    // Iniciar el servidor web
    startServer();

    while (1) {
        ESP_LOGI(TAG, "Server iniciado");
        vTaskDelay(2000 / portTICK_PERIOD_MS); // Espera 2 segundos entre cada impresión
    }
}
