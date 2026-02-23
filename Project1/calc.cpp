#include <graphics.h>   // EasyX 图形库（非标准）：窗口、绘图、文本、鼠标等 API
#include <conio.h>      // 控制台输入：_kbhit, _getch（MSVC/Windows 特有）
#include <stdio.h>      // 标准 I/O：sprintf 等（这里使用 _stprintf_s）
#include <string.h>     // 字符串处理：_tcscpy_s/_tcscat_s 等（Windows 泛型宏）
#include <tchar.h>      // TCHAR / _T() / _tcslen / _ttof 等通用文本宏
#include <stdlib.h>     // 标准库：atoi/atof 等（这里用 _ttof）
#include <ctype.h>      // 字符分类：_istdigit / _istspace
#include <windows.h>    // Sleep 等 Windows API（可选，用于定时）


/* 简要说明
   - 这是一个基于 EasyX 的计算器界面程序（Windows 平台）。
   - 使用纯 C 风格实现表达式解析与求值（支持 + - * /、小数、一元正负，不支持括号）。
   - 使用 TCHAR/_T() 系列以兼容 Unicode / ANSI 设置（在 VS 项目中由 Character Set 控制）。
*/

/* 窗口与控件尺寸配置（用于放大屏幕） */
const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 640;
const int LEFT_MARGIN = 20;
const int DISP_TOP = 50;
const int DISP_HEIGHT = 80;
const int DISP_RIGHT_MARGIN = 20;

/* 按钮尺寸与布局 */
const int BTN_WIDTH = 70;
const int BTN_HEIGHT = 60;
const int BTN_H_SPACING = 20;
const int BTN_V_SPACING = 20;
const int BTN_START_Y = DISP_TOP + DISP_HEIGHT + 30;

const int BUTTON_COUNT = 17; // 按钮数量：0-9，+，-，*，/，=，C，.

/* 按钮结构体（C 风格） */
typedef struct 
{
    int x, y, width, height; // 位置和尺寸
    TCHAR text[10];          // 按钮显示文本（TCHAR 兼容 Unicode/ANSI）
    int id;                  // 按钮 ID（可选）
} Button;

Button buttons[BUTTON_COUNT]; // 全部按钮数组
TCHAR input[100] = _T("");    // 显示缓冲区（用户输入的表达式或者结果）
int input_len = 0;            // 当前输入长度

/* 初始化所有按钮的位置和文本 */
void initButtons(void) 
{
    // 数字按钮按传统计算器排列，使用 3 列布局
    int digit_positions[][2] = { {1,0}, {0,1}, {1,1}, {2,1}, {0,2}, {1,2}, {2,2}, {0,3}, {1,3}, {2,3} };

    for (int i = 0; i < 10; i++) 
    {
        int col = digit_positions[i][0];
        int row = digit_positions[i][1];
        buttons[i].x = LEFT_MARGIN + col * (BTN_WIDTH + BTN_H_SPACING);
        buttons[i].y = BTN_START_Y + row * (BTN_HEIGHT + BTN_V_SPACING);
        buttons[i].width = BTN_WIDTH;
        buttons[i].height = BTN_HEIGHT;
        buttons[i].id = i;
        _stprintf_s(buttons[i].text, 10, _T("%d"), i); // 写入按钮文本 ("0".."9")
    }

    // 运算符按钮放在数字区右侧（4 个）
    int opX = LEFT_MARGIN + 3 * (BTN_WIDTH + BTN_H_SPACING);

    _tcscpy_s(buttons[10].text, 10, _T("+")); buttons[10].x = opX; buttons[10].y = BTN_START_Y + 0 * (BTN_HEIGHT + BTN_V_SPACING); buttons[10].width = BTN_WIDTH; buttons[10].height = BTN_HEIGHT; buttons[10].id = 10;
    _tcscpy_s(buttons[11].text, 10, _T("-")); buttons[11].x = opX; buttons[11].y = BTN_START_Y + 1 * (BTN_HEIGHT + BTN_V_SPACING); buttons[11].width = BTN_WIDTH; buttons[11].height = BTN_HEIGHT; buttons[11].id = 11;
    _tcscpy_s(buttons[12].text, 10, _T("*")); buttons[12].x = opX; buttons[12].y = BTN_START_Y + 2 * (BTN_HEIGHT + BTN_V_SPACING); buttons[12].width = BTN_WIDTH; buttons[12].height = BTN_HEIGHT; buttons[12].id = 12;
    _tcscpy_s(buttons[13].text, 10, _T("/")); buttons[13].x = opX; buttons[13].y = BTN_START_Y + 3 * (BTN_HEIGHT + BTN_V_SPACING); buttons[13].width = BTN_WIDTH; buttons[13].height = BTN_HEIGHT; buttons[13].id = 13;

    // 等号和清除按钮
    _tcscpy_s(buttons[14].text, 10, _T("=")); buttons[14].x = opX; buttons[14].y = BTN_START_Y + 4 * (BTN_HEIGHT + BTN_V_SPACING); buttons[14].width = BTN_WIDTH; buttons[14].height = BTN_HEIGHT; buttons[14].id = 14;
    _tcscpy_s(buttons[15].text, 10, _T("C")); buttons[15].x = LEFT_MARGIN; buttons[15].y = BTN_START_Y + 4 * (BTN_HEIGHT + BTN_V_SPACING); buttons[15].width = BTN_WIDTH; buttons[15].height = BTN_HEIGHT; buttons[15].id = 15;

    // 小数点按钮（放在底部中间）
    int dotCol = 1;
    int dotRow = 4;
    buttons[16].x = LEFT_MARGIN + dotCol * (BTN_WIDTH + BTN_H_SPACING);
    buttons[16].y = BTN_START_Y + dotRow * (BTN_HEIGHT + BTN_V_SPACING);
    buttons[16].width = BTN_WIDTH;
    buttons[16].height = BTN_HEIGHT;
    buttons[16].id = 16;
    _tcscpy_s(buttons[16].text, 10, _T("."));
}

/* 运算符优先级：* / 优先级为 2，+ - 为 1 */
int precedence(TCHAR op) 
{
    if (op == _T('*') || op == _T('/')) return 2;
    if (op == _T('+') || op == _T('-')) return 1;
    return 0;
}

/* evaluateExpression
   - 输入：expr（以 TCHAR 字符组成的表达式字符串）
   - 输出：outResult 指向的 double 写入计算结果
   - 返回：1 表示成功并写入结果；0 表示解析或计算失败（语法错误、除零等）
   说明（实现要点）：
   1) 词法分析（tokenize）：把表达式分解为数字 token 和运算符 token。支持小数点和一元 +/-
   2) 中缀转后缀（Shunting-yard algorithm）：把 tokens 转换为后缀（逆波兰）顺序，便于求值
   3) 求值后缀表达式：使用栈计算最终结果
*/
int evaluateExpression(const TCHAR* expr, double* outResult) 
{
    typedef struct { int isNumber; double value; TCHAR op; } Token;
    Token tokens[128]; // 词法 token 缓冲，固定大小（真实项目可改为动态）
    int tcount = 0;

    const TCHAR* p = expr;
    int expectNumber = 1; // 在表达式开始或上一个为运算符时期望数字（可以解析一元 +/-）

    while (*p) 
    {
        if (_istspace(*p)) 
        { 
            p++; // 忽略空白
            continue; 
        }

        // 处理一元 + 或 -（例如 "-3" 或 "+5"）
        if (((*p) == _T('+') || (*p) == _T('-')) && expectNumber) 
        {
            TCHAR sign = *p;
            p++;
            if (!_istdigit(*p) && *p != _T('.')) return 0; // 一元符号后必须是数字或小数点
            TCHAR buf[64]; int bi = 0;
            buf[bi++] = sign;
            while (*p && (_istdigit(*p) || *p == _T('.')) && bi < 63) buf[bi++] = *p++;
            buf[bi] = _T('\0');
            double v = _ttof(buf); // _ttof 会根据 TCHAR 显示映射到合适函数
            if (tcount < 128) { tokens[tcount].isNumber = 1; tokens[tcount].value = v; tokens[tcount].op = 0; tcount++; }
            expectNumber = 0;
            continue;
        }

        // 解析常规数值（可带小数点）
        if (_istdigit(*p) || *p == _T('.')) 
        {
            TCHAR buf[64]; int bi = 0;
            while (*p && (_istdigit(*p) || *p == _T('.')) && bi < 63) buf[bi++] = *p++;
            buf[bi] = _T('\0');
            double v = _ttof(buf);
            if (tcount < 128) { tokens[tcount].isNumber = 1; tokens[tcount].value = v; tokens[tcount].op = 0; tcount++; }
            expectNumber = 0;
            continue;
        }

        // 普通二元运算符
        if (*p == _T('+') || *p == _T('-') || *p == _T('*') || *p == _T('/')) 
        {
            if (tcount < 128) { tokens[tcount].isNumber = 0; tokens[tcount].value = 0.0; tokens[tcount].op = *p; tcount++; }
            p++;
            expectNumber = 1;
            continue;
        }

        // 遇到无法识别的字符 -> 语法错误
        return 0; 
    }

    if (tcount == 0) return 0; // 空表达式
    if (tokens[tcount - 1].isNumber == 0) return 0; // 不能以运算符结尾

    /* 中缀转后缀（Shunting-yard）实现（固定数组模拟输出和运算符栈） */
    Token output[128];
    int ocount = 0;
    TCHAR opstack[128];
    int op_top = -1;

    for (int i = 0; i < tcount; ++i) 
    {
        if (tokens[i].isNumber) 
        {
            if (ocount < 128) output[ocount++] = tokens[i];
        }
        else 
        {
            TCHAR op = tokens[i].op;
            // 把栈中优先级 >= 当前运算符的运算符弹到输出队列
            while (op_top >= 0 && precedence(opstack[op_top]) >= precedence(op)) 
            {
                if (ocount < 128) 
                { 
                    Token t; t.isNumber = 0; t.value = 0.0; t.op = opstack[op_top]; output[ocount++] = t; 
                }
                op_top--;
            }
            if (op_top < 127) 
                opstack[++op_top] = op;
        }
    }
    // 把剩余运算符弹出到输出
    while (op_top >= 0) 
    {
        if (ocount < 128) 
        { 
            Token t; t.isNumber = 0; t.value = 0.0; t.op = opstack[op_top]; output[ocount++] = t; 
        }
        op_top--;
    }

    /* 计算后缀表达式（使用栈） */
    double stk[128];
    int stk_top = -1;
    for (int i = 0; i < ocount; ++i) 
    {
        if (output[i].isNumber) 
        {
            if (stk_top < 127) 
                stk[++stk_top] = output[i].value;
        }
        else 
        {
            if (stk_top < 1) 
                return 0; // 操作数不足 -> 语法错误
            double b = stk[stk_top--];
            double a = stk[stk_top--];
            double r = 0.0;
            switch (output[i].op) 
            {
            case _T('+'): r = a + b; break;
            case _T('-'): r = a - b; break;
            case _T('*'): r = a * b; break;
            case _T('/'):
                if (b == 0.0) return 0; // 除零错误
                r = a / b; break;
            default: return 0;
            }
            if (stk_top < 127) stk[++stk_top] = r;
        }
    }

    if (stk_top != 0) return 0; // 最终栈应只剩一个结果
    *outResult = stk[stk_top];
    return 1;
}

/* 绘制计算器界面（每帧重绘） */
void drawCalculator(void) 
{
    cleardevice();

    // 绘制显示区背景（黑底）
    setfillcolor(BLACK);
    fillrectangle(LEFT_MARGIN, DISP_TOP, WINDOW_WIDTH - DISP_RIGHT_MARGIN, DISP_TOP + DISP_HEIGHT);

    // 绘制显示文本（白色，使用透明背景避免覆盖方块）
    settextcolor(WHITE);
    settextstyle(24, 0, _T("Consolas"));
    setbkmode(TRANSPARENT);

    outtextxy(LEFT_MARGIN + 8, DISP_TOP + (DISP_HEIGHT - textheight(input)) / 2, input);

    // 绘制所有按钮（背景、边框、文字）
    for (int i = 0; i < BUTTON_COUNT; i++) 
    {
        setfillcolor(LIGHTGRAY);
        fillroundrect(buttons[i].x, buttons[i].y,
            buttons[i].x + buttons[i].width,
            buttons[i].y + buttons[i].height, 8, 8);

        setlinecolor(DARKGRAY);
        roundrect(buttons[i].x, buttons[i].y,
            buttons[i].x + buttons[i].width,
            buttons[i].y + buttons[i].height, 8, 8);

        settextcolor(BLACK);
        settextstyle(18, 0, _T("Consolas"));
        setbkmode(TRANSPARENT);

        int text_x = buttons[i].x + (buttons[i].width - textwidth(buttons[i].text)) / 2;
        int text_y = buttons[i].y + (buttons[i].height - textheight(buttons[i].text)) / 2;
        outtextxy(text_x, text_y, buttons[i].text);
    }
}

/* 主循环：处理鼠标、键盘事件并刷新界面 */
int main(void) 
{
    // 初始化图形窗口（EasyX）
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);

    setbkcolor(WHITE);
    setbkmode(TRANSPARENT);
    cleardevice();

    initButtons();
    BeginBatchDraw(); // 开启批量绘制以减少闪烁

    MOUSEMSG m;

    while (1) 
    {
        // 处理鼠标点击（优先级高，及时响应）
        while (MouseHit()) 
        {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) 
            {
                for (int i = 0; i < BUTTON_COUNT; i++) 
                {
                    if (m.x >= buttons[i].x && m.x <= buttons[i].x + buttons[i].width && m.y >= buttons[i].y && m.y <= buttons[i].y + buttons[i].height)   
                    {
                        if (i == 15) 
                        { // C 按钮：清空输入
                            input_len = 0;
                            input[0] = _T('\0');
                        }
                        else if (i == 14) 
                        { // = 按钮：解析并计算表达式
                            double result = 0.0;
                            if (_tcslen(input) == 0) 
                            {
                                _tcscpy_s(input, 100, _T("")); input_len = 0;
                            }
                            else 
                            {
                                int ok = evaluateExpression(input, &result);
                                if (ok) 
                                {
                                    // 使用 %g 格式避免多余的 .000
                                    _stprintf_s(input, 100, _T("%g"), result);
                                    input_len = (int)_tcslen(input);
                                }
                                else 
                                {
                                    _tcscpy_s(input, 100, _T("错误"));
                                    input_len = (int)_tcslen(input);
                                }
                            }
                        }
                        else 
                        {
                            // 追加按钮文本到输入（数字、运算符、小数点）
                            if (input_len < 99) 
                            {
                                _tcscat_s(input, 100, buttons[i].text);
                                input_len = (int)_tcslen(input);
                            }
                        }
                        break; // 已处理当前点击，退出按钮循环
                    }
                }
            }
        }

        // 处理键盘输入（非阻塞）
        if (_kbhit()) 
        {
            int ch = _getch();
            if (ch == 27) 
                break; // ESC 退出
            else if (ch == 8 && input_len > 0) 
            { // 退格
                input[--input_len] = _T('\0');
            }
            else if ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '.') 
            {
                // 追加字符到输入
                if (input_len < 99) 
                {
                    TCHAR chStr[2];
                    chStr[0] = (TCHAR)ch;
                    chStr[1] = _T('\0');
                    _tcscat_s(input, 100, chStr);
                    input_len = (int)_tcslen(input);
                }
            }
            else if (ch == '=' || ch == '\r') 
            {
                // 回车或等号触发计算（同鼠标 = 按钮）
                double result = 0.0;
                if (_tcslen(input) == 0) 
                {
                    _tcscpy_s(input, 100, _T("")); input_len = 0;
                }
                else 
                {
                    int ok = evaluateExpression(input, &result);
                    if (ok) 
                    {
                        _stprintf_s(input, 100, _T("%g"), result);
                        input_len = (int)_tcslen(input);
                    }
                    else 
                    {
                        _tcscpy_s(input, 100, _T("错误"));
                        input_len = (int)_tcslen(input);
                    }
                }
            }
            else if (ch == 'c' || ch == 'C') 
            {
                // 键盘 C 清除
                input_len = 0;
                input[0] = _T('\0');
            }
        }

        // 绘制并刷新（批量绘制模式）
        drawCalculator();
        FlushBatchDraw();
        Sleep(10); // 小延迟，防止 CPU 占用过高
    }

    // 退出清理
    EndBatchDraw();
    closegraph();
    return 0;
}