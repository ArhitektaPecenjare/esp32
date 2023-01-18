/* TWAI Network Listen Only Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * The following example demonstrates a Listen Only node in a TWAI network. The
 * Listen Only node will not take part in any TWAI bus activity (no acknowledgments
 * and no error frames). This example will execute multiple iterations, with each
 * iteration the Listen Only node will do the following:
 * 1) Listen for ping and ping response
 * 2) Listen for start command
 * 3) Listen for data messages
 * 4) Listen for stop and stop response
 */
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

/* --------------------- Definitions and static variables ------------------ */
//Example Configuration
#define NO_OF_ITERS                     30000
#define RX_TASK_PRIO                    9
#define TX_GPIO_NUM                     5
#define RX_GPIO_NUM                     4
#define EXAMPLE_TAG                     "TWAI Listen Only"

#define ID_MASTER_STOP_CMD              0x0A0
#define ID_MASTER_START_CMD             0x0A1
#define ID_MASTER_PING                  0x0A2
#define ID_SLAVE_STOP_RESP              0x0B0
#define ID_SLAVE_DATA                   0x0B1
#define ID_SLAVE_PING_RESP              0x0B2


static SemaphoreHandle_t rx_sem;

/* --------------------------- Tasks and Functions -------------------------- */

static void twai_receive_task(void *arg)
{
    bool start_cmd = false;
    bool stop_resp = false;
    uint32_t iterations = 0;
esp_err_t e = 0;
    while (iterations < NO_OF_ITERS) {
        twai_message_t message;
        e = twai_receive(&message, pdMS_TO_TICKS(1000));
        if ( e == ESP_OK) {
            ESP_LOGI(EXAMPLE_TAG, "Message received\n");
        } else {
            ESP_LOGI(EXAMPLE_TAG, "Failed to receive message\n %d", e);
            return;
        }

        //Process received message
        if (message.extd) {
            ESP_LOGI(EXAMPLE_TAG, "Message is in Extended Format\n");
        } else {
            printf("Message is in Standard Format\n");
        }
        ESP_LOGI(EXAMPLE_TAG, "ID is %d\n", message.identifier);
        if (!(message.rtr)) {
            for (int i = 0; i < message.data_length_code; i++) {
                ESP_LOGI(EXAMPLE_TAG, "Data byte %d = %d\n", i, message.data[i]);
            }
        }

    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    rx_sem = xSemaphoreCreateBinary();

    //Install and start TWAI driver
    // ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    // ESP_LOGI(EXAMPLE_TAG, "Driver installed");
    // ESP_ERROR_CHECK(twai_start());
    // ESP_LOGI(EXAMPLE_TAG, "Driver started");
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
    twai_filter_config_t f_config = {.acceptance_code = 0, .acceptance_mask = 0, .single_filter = true}; //TWAI_FILTER_CONFIG_ACCEPT_ALL();

    //Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }


    vTaskDelay(pdMS_TO_TICKS(100));

    xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
    
    vTaskDelay(pdMS_TO_TICKS(100));
    // //Stop and uninstall TWAI driver
    // ESP_ERROR_CHECK(twai_stop());
    // ESP_LOGI(EXAMPLE_TAG, "Driver stopped");
    // ESP_ERROR_CHECK(twai_driver_uninstall());
    // ESP_LOGI(EXAMPLE_TAG, "Driver uninstalled");

    //Cleanup
    vSemaphoreDelete(rx_sem);
}
