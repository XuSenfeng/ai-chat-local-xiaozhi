#include "chat_ai.h"
#include "settings.h"

#include <esp_log.h>
#include <esp_system.h>

#include "cJSON.h"
#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#if CONFIG_USE_CHAT_LOCAL | CONFIG_USE_CHAT_DIFY
#define TAG "Chat_ai"
#endif

#if CONFIG_USE_CHAT_LOCAL

#define POST_DATA    "{\
\"messages\":[{\"sender_type\":\"USER\", \"user_id\":%d,\"sender_name\":\"test\",\"text\":\"%s\"}]\
}"

char chat_local_content[2048]={0};
int user_id  = -1;
Chat_ai::Chat_ai(){
    Settings settings("ai_chat", true);
    user_id = settings.GetInt("user_id", -1);
    voice = 0;
    
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
    int post_len = sprintf(post_buffer, POST_DATA, user_id, text); //动态获取一内存记录信息
    

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
#endif 



#if CONFIG_USE_CHAT_DIFY

#define POST_DATA    "{\
    \"inputs\": {\"text\": \"%s\"},\
    \"response_mode\": \"blocking\",\
    \"user\": \"abc-123\"\
}"

char chat_local_content[2048]={0};

Chat_ai::Chat_ai(){
    
}
Chat_ai::~Chat_ai(){

}
void decode_unicode(char *encoded_str, char *decoded_str) {  
    char temp[7] = {'\0'};  
    int value;  
    int i = 0, j = 0;  

    while(encoded_str[i] != '\0') {  
        if(encoded_str[i] == '\\' && encoded_str[i + 1] == 'u') {  
            // 提取Unicode编码值  
            strncpy(temp, &(encoded_str[i + 2]), 4);  
            sscanf(temp, "%x", &value);  

            // 将Unicode编码值转换为字符并存入decoded_str  
            decoded_str[j] = (char)value;  
            j++;  
            i += 6;  
        } else {  
            decoded_str[j] = encoded_str[i];  
            j++;  
            i++;  
        }  
    }  

    decoded_str[j] = '\0';  
} 


int get_utf8_char_len(unsigned char c) {  
    if ((c & 0x80) == 0x00) {  
        return 1;  
    } else if ((c & 0xE0) == 0xC0) {  
        return 2;  
    } else if ((c & 0xF0) == 0xE0) {  
        return 3;  
    } else if ((c & 0xF8) == 0xF0) {  
        return 4;  
    } else {  
        // Invalid UTF-8 character  
        return 1;  
    }  
}  

void add_newline(char *str, char *new_str) {  
    int len = strlen(str);  
    int count = 0;  
    int new_index = 0;  
    int utf8_char_len = 0;  
    
    for (int i = 0; i < len; i += utf8_char_len) {  
        utf8_char_len = get_utf8_char_len((unsigned char)str[i]);  
        
        for (int j = 0; j < utf8_char_len; j++) {  
            new_str[new_index++] = str[i + j];  
        }  
        
        count++;  
        
        if (count == 12) {  
            new_str[new_index] = '\n';  
            new_index++;  
            count = 0;  
        }  
    }  
    
    new_str[new_index] = '\0';  
}  


void Chat_ai_Comunicate_task(void * Param){
    message_t *msg = (message_t *)Param;
    const char *text = msg->text;
    Display *display = msg->display;
    ESP_LOGI(TAG, "text:%s", text);
    char *response_text = NULL;
    char *post_buffer = NULL;
    char *data_buf = NULL; 

    display->SetChatStatus(1);

    esp_http_client_config_t config = {
        .url = CONFIG_CHAT_AI_DIFY_URL,  // 这里替换成自己的GroupId
        .timeout_ms = 40000,
        .buffer_size_tx = 1024  // 默认是512 minimax_key很长 512不够 这里改成1024
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    post_buffer = (char *)malloc(MAX_CHAT_BUFFER);
    
    if (post_buffer == NULL) {
        esp_http_client_cleanup(client);
        display->SetChatStatus(0);
        vTaskDelete(NULL);
    }
    int post_len = sprintf(post_buffer, POST_DATA, text); //动态获取一内存记录信息
    

    // POST
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", CONFIG_CHAT_AI_DIFY_API_KEY);

    if (esp_http_client_open(client, post_len) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening connection");
        free(post_buffer);
        esp_http_client_cleanup(client);
        display->SetChatStatus(0);
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
        display->SetChatStatus(0);
        vTaskDelete(NULL);
    }
    data_buf[data_length] = '\0';
    int rlen = esp_http_client_read(client, data_buf, data_length);
    data_buf[rlen] = '\0';
    ESP_LOGI(TAG, "read = %s", data_buf);
    //解析信息
    cJSON *root = cJSON_Parse(data_buf);
    cJSON *data_j = cJSON_GetObjectItem(root, "data");
    if(data_j == 0)
    {
        cJSON_Delete(root);
        free(data_buf);
        free(post_buffer);
        esp_http_client_cleanup(client);
        ESP_LOGE(TAG, "data_j is null");
        display->SetChatStatus(0);
        vTaskDelete(NULL);
    }
    // 获取status字段判断是否成功
    char * status = cJSON_GetObjectItem(data_j,"status")->valuestring;
    if(status == 0)
    {
        cJSON_Delete(root);
        free(data_buf);
        free(post_buffer);
        esp_http_client_cleanup(client);
        ESP_LOGE(TAG, "status is null");
        display->SetChatStatus(0);
        vTaskDelete(NULL);
    }
    if(strcmp(status, "failed") == 0)
    {
        cJSON_Delete(root);
        free(data_buf);
        free(post_buffer);
        esp_http_client_cleanup(client);
        ESP_LOGE(TAG, "status is failed");
        display->SetChatStatus(0);
        vTaskDelete(NULL);
    }
    cJSON *outputs_j = cJSON_GetObjectItem(data_j, "outputs");
    if(outputs_j == 0)
    {
        cJSON_Delete(root);
        free(data_buf);
        free(post_buffer);
        esp_http_client_cleanup(client);
        ESP_LOGE(TAG, "outputs_j is null");
        display->SetChatStatus(0);
        vTaskDelete(NULL);
    }
    char * tool_result = cJSON_GetObjectItem(outputs_j,"text")->valuestring;
    if(tool_result != 0)
    {
        // ESP_ERR_NVS_INVALID_HANDLE 
        char *temp_buf_utf = (char *)malloc(2048);
        decode_unicode(tool_result, temp_buf_utf);
        add_newline(temp_buf_utf, chat_local_content);
        free(temp_buf_utf);
        response_text = chat_local_content;
        ESP_LOGI(TAG, "response_text:%s", response_text);
    }

    cJSON_Delete(root);
    free(data_buf);
    free(post_buffer);
    esp_http_client_cleanup(client);


    display->SetChatMessageTool("tool", response_text);
    display->SetChatStatus(2);
    vTaskDelete(NULL);
}

#endif

#if CONFIG_USE_CHAT_LOCAL | CONFIG_USE_CHAT_DIFY

void Chat_ai::Chat_ai_Comunicate(message_t *msg){
    xTaskCreate(Chat_ai_Comunicate_task, "Chat_ai_Comunicate_task", 4096, (void *)msg, 5, NULL);
}

#endif


