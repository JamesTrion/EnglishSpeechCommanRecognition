#include "app_audio.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "AUDIO_SR";
static speech_cmd_cb_t g_cmd_callback = NULL;

// 定義 7 秒逾時時間 (7000 ms)
#define LISTENING_TIMEOUT_MS 7000

// 系統語音識別狀態
typedef enum {
    STATE_WAIT_WAKEUP,  // 等待喚醒詞 "Hi ESP"
    STATE_LISTENING     // 7 秒指令接收視窗中
} voice_system_state_t;

static voice_system_state_t g_current_state = STATE_WAIT_WAKEUP;
static TickType_t g_last_wakeup_tick = 0;

/**
 * Task 1: 音訊讀取與輸入 Task (維持不變)
 */
static void feed_task(void *arg) {
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;
    int audio_chunksize = esp_afe_sr_v1.get_feed_chunksize(afe_data);
    int16_t *feed_buff = malloc(audio_chunksize * sizeof(int16_t) * 2);

    while (1) {
        bsp_audio_get_feed_data(feed_buff, audio_chunksize * sizeof(int16_t) * 2);
        esp_afe_sr_v1.feed(afe_data, feed_buff);
    }
    free(feed_buff);
    vTaskDelete(NULL);
}

/**
 * Task 2: 包含 7 秒逾時控制邏輯的語音識別 Task
 */
static void detect_task(void *arg) {
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *)arg;
    esp_mn_iface_t *multinet = &MULTINET_MODEL;
    model_iface_data_t *model_data = multinet->create(&MULTINET_COEFF, 4000);

    // 清除並設定中文語音指令
    esp_mn_commands_clear();
    esp_mn_commands_add(VOICE_CMD_COPY,       "fu zhi");      // 複製
    esp_mn_commands_add(VOICE_CMD_PASTE,      "tie shang");   // 貼上
    esp_mn_commands_add(VOICE_CMD_UNDO,       "fu yuan");     // 復原
    esp_mn_commands_add(VOICE_CMD_SAVE,       "cun dang");    // 存檔
    esp_mn_commands_add(VOICE_CMD_SELECT_ALL, "quan xuan");   // 全選
    esp_mn_commands_update();

    ESP_LOGI(TAG, "Audio SR initialized. Waiting for wake word 'Hi ESP'...");

    while (1) {
        afe_fetch_result_t* res = esp_afe_sr_v1.fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            continue;
        }

        // -------------------------------------------------------------
        // 情況 1：檢測到喚醒詞 (Wake Word)
        // -------------------------------------------------------------
        if (res->wakeup_state == WAKENED) {
            g_current_state = STATE_LISTENING;
            g_last_wakeup_tick = xTaskGetTickCount(); // 紀錄喚醒時間點
            
            ESP_LOGI(TAG, ">>> [系統喚醒] 請在 7 秒之內說出指令 (例如：複製、貼上) <<<");
            // 💡 提示：可以在這裡加上 BOX-3 的發聲提示音或 UI 畫面變更
            continue;
        }

        // -------------------------------------------------------------
        // 情況 2：處於 7 秒 Listening 視窗中，嘗試識別指令
        // -------------------------------------------------------------
        if (g_current_state == STATE_LISTENING) {
            
            // A. 檢查是否已經超過 7 秒未收到有效的指令
            TickType_t current_tick = xTaskGetTickCount();
            if ((current_tick - g_last_wakeup_tick) > pdMS_TO_TICKS(LISTENING_TIMEOUT_MS)) {
                g_current_state = STATE_WAIT_WAKEUP;
                ESP_LOGW(TAG, ">>> [7秒逾時] 自動停止接收指令，退回等待喚醒模式 <<<");
                continue;
            }

            // B. 進行 Multinet 指令檢測
            int cmd_id = multinet->detect(model_data, res->data);
            if (cmd_id > 0) {
                ESP_LOGI(TAG, "收到有效指令 ID: %d", cmd_id);
                
                // 觸發鍵盤按鍵 (主程式 Callback)
                if (g_cmd_callback) {
                    g_cmd_callback((voice_command_id_t)cmd_id);
                }

                // 模式選擇 (請擇一使用)：
                // 選擇【選項 A】：收到指令後「重置 7 秒」，方便連續說指令 (如「複製」->「貼上」)
                g_last_wakeup_tick = xTaskGetTickCount();
                ESP_LOGI(TAG, "指令執行完畢，7秒視窗已重置，繼續等待下一個指令...");

                /* 
                // 選擇【選項 B】：收到指令後「立刻退出」，下次要再說指令必須重新喊啟動語
                g_current_state = STATE_WAIT_WAKEUP;
                ESP_LOGI(TAG, "指令執行完畢，退出聆聽。");
                */
            }
        }
    }
    vTaskDelete(NULL);
}

void app_audio_sr_init(speech_cmd_cb_t callback) {
    g_cmd_callback = callback;

    // 初始化 BOX-3 核心板載音訊硬體
    bsp_audio_init();

    // 配置 ESP-SR 音訊前端 (AFE)
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.wakenet_init = true;            // 必須啟用喚醒詞檢測
    afe_config.voice_communication_init = false;
    
    esp_afe_sr_data_t *afe_data = esp_afe_sr_v1.create_from_config(&afe_config);

    // 建立 Task
    xTaskCreatePinnedToCore(feed_task, "feed_task", 8192, afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(detect_task, "detect_task", 8192, afe_data, 5, NULL, 1);
}