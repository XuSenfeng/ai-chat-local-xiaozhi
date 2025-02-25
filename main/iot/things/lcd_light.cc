#include "iot/thing.h"
#include "board.h"
#include "display.h"

#include <esp_log.h>
#if CONFIG_USE_PERSONALIZED
#define TAG "LcdLight"

namespace iot {

// 这里仅定义 Speaker 的属性和方法，不包含具体的实现
class LcdLight: public Thing {
public:
    LcdLight() : Thing("LcdLight", "当前屏幕的亮度") {
        // 定义设备的属性
        properties_.AddNumberProperty("light", "当前亮度值", [this]() -> int {
            auto display = Board::GetInstance().GetDisplay();
            return display->DisplayBrightnessGetDefalutLight();
        });

        // 定义设备可以被远程执行的指令
        methods_.AddMethod("SetLight", "设置屏幕亮度", ParameterList({
            Parameter("light", "0到100之间的整数", kValueTypeNumber, true)
        }), [this](const ParameterList& parameters) {
            auto display = Board::GetInstance().GetDisplay();
            display->SetBacklight(static_cast<uint8_t>(parameters["light"].number()));
            // printf("SetLight: %d\n", parameters["light"].number());
        });
    }
};

} // namespace iot

DECLARE_THING(LcdLight);
#endif
