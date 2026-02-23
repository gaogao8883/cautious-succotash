#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

// 窗口与控件尺寸配置（用于放大屏幕）
const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 640;
const int LEFT_MARGIN = 20;
const int DISP_TOP = 50;
const int DISP_HEIGHT = 110; // 增大显示屏高度
const int DISP_RIGHT_MARGIN = 20;

const int BTN_WIDTH = 70;
const int BTN_HEIGHT = 64;   // 略微增大按钮高度
const int BTN_H_SPACING = 18;
const int BTN_V_SPACING = 18;
const int BTN_START_Y = DISP_TOP + DISP_HEIGHT + 24;

// 按钮结构体
struct Button {
    int x, y, width, height;
    char text[10];
    int id;
};

Button buttons[16]; // 0-9, +, -, *, /, =, C
char input[100] = "";
int input_len = 0;
bool needRedraw = true; // 只在需要时重绘，避免闪烁

void initButtons() {
    // 初始化数字按钮（3列）
    int digit_positions[][2] = { {1,0}, {0,1}, {1,1}, {2,1}, {0,2}, {1,2}, {2,2}, {0,3}, {1,3}, {2,3} };

    for (int i = 0; i < 10; i++) {
        int col = digit_positions[i][0];
        int row = digit_positions[i][1];
        buttons[i].x = LEFT_MARGIN + col * (BTN_WIDTH + BTN_H_SPACING);
        buttons[i].y = BTN_START_Y + row * (BTN_HEIGHT + BTN_V_SPACING);
        buttons[i].width = BTN_WIDTH;
        buttons[i].height = BTN_HEIGHT;
        buttons[i].id = i;
        sprintf_s(buttons[i].text, sizeof(buttons[i].text), "%d", i);
    }

    // 运算符按钮（放在数字区右侧）
    int opX = LEFT_MARGIN + 3 * (BTN_WIDTH + BTN_H_SPACING);

    buttons[10].x = opX; buttons[10].y = BTN_START_Y + 0 * (BTN_HEIGHT + BTN_V_SPACING); buttons[10].width = BTN_WIDTH; buttons[10].height = BTN_HEIGHT; buttons[10].id = 10; strcpy_s(buttons[10].text, sizeof(buttons[10].text), "+");
    buttons[11].x = opX; buttons[11].y = BTN_START_Y + 1 * (BTN_HEIGHT + BTN_V_SPACING); buttons[11].width = BTN_WIDTH; buttons[11].height = BTN_HEIGHT; buttons[11].id = 11; strcpy_s(buttons[11].text, sizeof(buttons[11].text), "-");
    buttons[12].x = opX; buttons[12].y = BTN_START_Y + 2 * (BTN_HEIGHT + BTN_V_SPACING); buttons[12].width = BTN_WIDTH; buttons[12].height = BTN_HEIGHT; buttons[12].id = 12; strcpy_s(buttons[12].text, sizeof(buttons[12].text), "*");
    buttons[13].x = opX; buttons[13].y = BTN_START_Y + 3 * (BTN_HEIGHT + BTN_V_SPACING); buttons[13].width = BTN_WIDTH; buttons[13].height = BTN_HEIGHT; buttons[13].id = 13; strcpy_s(buttons[13].text, sizeof(buttons[13].text), "/");

    // 等号与清除按钮
    buttons[14].x = opX; buttons[14].y = BTN_START_Y + 4 * (BTN_HEIGHT + BTN_V_SPACING); buttons[14].width = BTN_WIDTH; buttons[14].height = BTN_HEIGHT; buttons[14].id = 14; strcpy_s(buttons[14].text, sizeof(buttons[14].text), "=");
    buttons[15].x = LEFT_MARGIN; buttons[15].y = BTN_START_Y + 4 * (BTN_HEIGHT + BTN_V_SPACING); buttons[15].width = BTN_WIDTH; buttons[15].height = BTN_HEIGHT; buttons[15].id = 15; strcpy_s(buttons[15].text, sizeof(buttons[15].text), "C");
}

void drawCalculator() {
    // 只有在 needRedraw == true 时调用（由外部控制）
    cleardevice();

    // 绘制显示屏背景
    setfillcolor(BLACK);
    fillrectangle(LEFT_MARGIN, DISP_TOP, WINDOW_WIDTH - DISP_RIGHT_MARGIN, DISP_TOP + DISP_HEIGHT);

    // 绘制显示屏文字（透明背景，避免覆盖）
    settextcolor(WHITE);
    settextstyle(40, 0, _T("Consolas")); // 数字更大
    setbkmode(TRANSPARENT); // 关键：文本背景透明，避免出现方块

    // 将 char* input 转换为 LPCTSTR 并显示，使用右对齐以仿真计算器
#ifdef UNICODE
    wchar_t winput[100];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, winput, 100, input, _TRUNCATE);
    int tw = textwidth(winput);
    int tx = (WINDOW_WIDTH - DISP_RIGHT_MARGIN) - tw - 8; // 右对齐，保留 8px 右边距
    int ty = DISP_TOP + (DISP_HEIGHT - textheight(winput)) / 2;
    outtextxy(tx, ty, winput);
#else
    int tw = textwidth(input);
    int tx = (WINDOW_WIDTH - DISP_RIGHT_MARGIN) - tw - 8; // 右对齐
    int ty = DISP_TOP + (DISP_HEIGHT - textheight(input)) / 2;
    outtextxy(tx, ty, input);
#endif

    // 绘制按钮
    for (int i = 0; i < 16; i++) {
        // 按钮背景
        setfillcolor(LIGHTGRAY);
        fillroundrect(buttons[i].x, buttons[i].y,
            buttons[i].x + buttons[i].width,
            buttons[i].y + buttons[i].height, 10, 10);

        // 按钮边框
        setlinecolor(DARKGRAY);
        roundrect(buttons[i].x, buttons[i].y,
            buttons[i].x + buttons[i].width,
            buttons[i].y + buttons[i].height, 10, 10);

        // 按钮文字 - 使用透明背景，避免覆盖按钮背景
        settextcolor(BLACK);
        settextstyle(24, 0, _T("Consolas")); // 按钮文字更大
        setbkmode(TRANSPARENT);

#ifdef UNICODE
        wchar_t wtext[10];
        size_t convertedChars = 0;
        mbstowcs_s(&convertedChars, wtext, 10, buttons[i].text, _TRUNCATE);
        int text_x = buttons[i].x + (buttons[i].width - textwidth(wtext)) / 2;
        int text_y = buttons[i].y + (buttons[i].height - textheight(wtext)) / 2;
        outtextxy(text_x, text_y, wtext);
#else
        int text_x = buttons[i].x + (buttons[i].width - textwidth(buttons[i].text)) / 2;
        int text_y = buttons[i].y + (buttons[i].height - textheight(buttons[i].text)) / 2;
        outtextxy(text_x, text_y, buttons[i].text);
#endif
    }
}

int main() {
    // 初始化图形窗口
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);

    // 窗口背景保持白色（总体背景），文本绘制使用透明背景，避免文本覆盖产生方块
    setbkcolor(WHITE);
    setbkmode(TRANSPARENT);
    cleardevice();

    initButtons();

    // 启用批量绘制以减少闪烁与提升响应
    BeginBatchDraw();

    MOUSEMSG m; // 鼠标消息

    // 首次强制绘制
    needRedraw = true;
    drawCalculator();
    FlushBatchDraw();
    needRedraw = false;

    while (true) {
        bool stateChanged = false;

        // 处理所有鼠标事件（非阻塞）
        while (MouseHit()) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                for (int i = 0; i < 16; i++) {
                    if (m.x >= buttons[i].x && m.x <= buttons[i].x + buttons[i].width &&
                        m.y >= buttons[i].y && m.y <= buttons[i].y + buttons[i].height) {

                        if (i == 15) { // C按钮
                            input_len = 0;
                            input[0] = '\0';
                            stateChanged = true;
                        }
                        else if (i == 14) { // =按钮
                            strcpy_s(input, sizeof(input), "计算中...");
                            input_len = static_cast<int>(strlen(input));
                            stateChanged = true;
                        }
                        else {
                            if (input_len < (int)sizeof(input) - 1) {
                                // 追加单字符，避免 strcat_s 对齐问题
                                size_t addLen = strlen(buttons[i].text);
                                if (input_len + (int)addLen < (int)sizeof(input) - 1) {
                                    strcat_s(input, sizeof(input), buttons[i].text);
                                    input_len = static_cast<int>(strlen(input));
                                    stateChanged = true;
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }

        // 键盘输入处理
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 27) break; // ESC退出
            else if (ch == 8 && input_len > 0) { // 退格键
                input[--input_len] = '\0';
                stateChanged = true;
            }
            else if ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '*' || ch == '/') {
                if (input_len < (int)sizeof(input) - 1) {
                    input[input_len++] = ch;
                    input[input_len] = '\0';
                    stateChanged = true;
                }
            }
            else if (ch == '=' || ch == '\r') { // 回车或等号
                strcpy_s(input, sizeof(input), "计算中...");
                input_len = static_cast<int>(strlen(input));
                stateChanged = true;
            }
            else if (ch == 'c' || ch == 'C') { // 清除
                input_len = 0;
                input[0] = '\0';
                stateChanged = true;
            }
        }

        // 只有状态变化时才重绘，减少闪烁
        if (stateChanged) {
            needRedraw = true;
        }

        if (needRedraw) {
            drawCalculator();
            FlushBatchDraw();
            needRedraw = false;
        }

        // 极小等待，降低延迟感知（可调整）
        Sleep(4);
    }

    // 结束批量绘制并关闭图形
    EndBatchDraw();
    closegraph();
    return 0;
}