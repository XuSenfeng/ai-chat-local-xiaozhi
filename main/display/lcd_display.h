#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "display.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>
#include <font_emoji.h>
#include <lvgl.h>
#include <atomic>

class LcdDisplay : public Display {
protected:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    gpio_num_t backlight_pin_ = GPIO_NUM_NC;
    bool backlight_output_invert_ = false;
    
    lv_draw_buf_t draw_buf_;
    lv_obj_t* status_bar_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_obj_t* container_ = nullptr;
    lv_obj_t* side_bar_ = nullptr;
    lv_obj_t* chat_message_label_ = nullptr;

#if CONFIG_USE_CHAT_LOCAL | CONFIG_USE_CHAT_DIFY
    lv_obj_t* chat_message_label_tool = nullptr;
    lv_obj_t* chat_message_lcd = nullptr;
#endif
#if CONFIG_USE_PERSONALIZED
    esp_timer_handle_t my_timer;
#endif

    DisplayFonts fonts_;

    void InitializeBacklight(gpio_num_t backlight_pin);
    virtual void SetupUI();
public:
    LcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                  gpio_num_t backlight_pin, bool backlight_output_invert,
                  int width, int height,  int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy,
                  DisplayFonts fonts);
    ~LcdDisplay();
#if CONFIG_USE_PERSONALIZED
    void SetBacklight(uint8_t brightness);
#else 
    void SetBacklight(uint8_t brightness) override;
#endif
    void SetChatMessage(const std::string &role, const std::string &content) override;
    void SetEmotion(const std::string &emotion) override;
    void SetIcon(const char* icon) override;
    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;
#if CONFIG_USE_CHAT_LOCAL | CONFIG_USE_CHAT_DIFY
    void SetChatMessageTool(const std::string &role, const std::string &content) override;
    void SetChatStatus(int status) override;
    void Change_show();
#endif
#if CONFIG_USE_PERSONALIZED
    void DisplayBrightnessReset(void);
    void DisplayBrightnessSetDefalutTime(int time);
    void DisplayBrightnessSetDefalutLight(int light);
    int DisplayBrightnessGetDefalutLight(void);
    int DisplayBrightnessGetDefalutTime(void);
    void DisplayBrightnessKeep(void);
    int brightness_time = CONFIG_LCD_LIGHT_TIME;  // 初识亮屏时间
    int default_sleep_time = CONFIG_LCD_LIGHT_TIME;    // 默认休眠时间
    int defalut_light = CONFIG_LCD_LIGHT_VALUE;    // 默认亮度
#endif
};

#if CONFIG_USE_PERSONALIZED
void DisplayBrightnessTask(void *arg);
#endif

#endif // LCD_DISPLAY_H
