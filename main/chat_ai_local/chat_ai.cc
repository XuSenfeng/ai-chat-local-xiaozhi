#include "chat_ai.h"
#include "settings.h"

#include <esp_log.h>
#include <esp_system.h>

#include "cJSON.h"
#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if CONFIG_USE_CHAT_LOCAL
#define TAG "Chat_ai"

#define PSOT_DATA    "{\
\"messages\":[{\"sender_type\":\"USER\", \"user_id\":%d,\"sender_name\":\"test\",\"text\":\"%s\"}]\
}"

char chat_local_content[2048]={0};
int user_id  = -1;
Chat_ai::Chat_ai(){
    Settings settings("ai_chat", true);
    user_id = settings.GetInt("user_id", -1);
    
}
Chat_ai::~Chat_ai(){

}
void Chat_ai_Comunicate_task(void * Param){
    message_t *msg = (message_t *)Param;
    const char *text = msg->text;
    Display *display = msg->display;
    ESP_LOGI(TAG, "text:%s", text);
    char *response_text = NULL;
    char *post_buffer = NULL;
    char *data_buf = NULL; 

    esp_http_client_config_t config = {
        .url = CONFIG_CHAT_AI_LOCAL_URL,  // 这里替换成自己的GroupId
        .timeout_ms = 40000,
        .buffer_size_tx = 1024  // 默认是512 minimax_key很长 512不够 这里改成1024
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    post_buffer = (char *)malloc(MAX_CHAT_BUFFER);
    
    if (post_buffer == NULL) {
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
    }
    int post_len = sprintf(post_buffer, PSOT_DATA, user_id, text); //动态获取一内存记录信息
    

    // POST
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    if (esp_http_client_open(client, post_len) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening connection");
        free(post_buffer);
        esp_http_client_cleanup(client);
    
        vTaskDelete(NULL);
    }
    int write_len = esp_http_client_write(client, post_buffer, post_len);
    ESP_LOGI(TAG, "Need to write %d, written %d", post_len, write_len);
    //获取信息长度
    int data_length = esp_http_client_fetch_headers(client);
    if (data_length <= 0) {
        data_length = MAX_CHAT_BUFFER;
    }
    //分配空间
    data_buf = (char *)malloc(data_length + 1);
    if(data_buf == NULL) {
        free(post_buffer);
        esp_http_client_cleanup(client);
        vTaskDelete(NULL);
    }
    data_buf[data_length] = '\0';
    int rlen = esp_http_client_read(client, data_buf, data_length);
    data_buf[rlen] = '\0';
    ESP_LOGI(TAG, "read = %s", data_buf);
    //解析信息
    cJSON *root = cJSON_Parse(data_buf);
    char * created = cJSON_GetObjectItem(root,"result")->valuestring;
    int32_t temp_id = cJSON_GetObjectItem(root,"user_id")->valueint;
    int32_t use_tool = cJSON_GetObjectItem(root,"tool")->valueint;
    if (temp_id != user_id)
    {
        user_id = temp_id;
        Settings settings("ai_chat", true);
        settings.SetInt("user_id", user_id);
        ESP_LOGI(TAG, "user_id:%d", user_id);
    }
    if(created != 0)
    {
        // ESP_ERR_NVS_INVALID_HANDLE 
        strcpy(chat_local_content, created);
        response_text = chat_local_content;
        ESP_LOGI(TAG, "response_text:%s", response_text);
    }

    cJSON_Delete(root);
    free(data_buf);
    free(post_buffer);
    esp_http_client_cleanup(client);

    if(use_tool){
        display->SetChatMessageTool("tool", response_text);
    }
    vTaskDelete(NULL);
}

void Chat_ai::Chat_ai_Comunicate(message_t *msg){
    xTaskCreate(Chat_ai_Comunicate_task, "Chat_ai_Comunicate_task", 3072, (void *)msg, 5, NULL);
}


#endif