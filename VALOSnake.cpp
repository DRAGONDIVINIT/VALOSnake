#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <windows.h>
#include <conio.h>

using namespace std;

// 游戏常量
const int WIDTH = 40;
const int HEIGHT = 20;

// 设置光标位置
void SetCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 隐藏光标
void HideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// 显示光标
void ShowCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// 方向枚举
enum Direction { UP, DOWN, LEFT, RIGHT, STOP };

// 点结构体
struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// 食物结构体，包含位置和生成时间
struct Food {
    Point position;
    clock_t spawnTime;  // 食物生成的时间
    
    Food(const Point& pos = Point()) : position(pos) {
        spawnTime = clock();  // 记录生成时间
    }
    
    bool operator==(const Food& other) const {
        return position == other.position;
    }
};

// 角色枚举
enum Role { NORMAL_ROLE, CLEAR_ROLE, REVIVE_ROLE };

// 贪吃蛇类
class SnakeGame {
private:
    vector<Point> snake;           // 蛇的身体
    vector<Food> foods;            // 食物列表（包含时间信息）
    Direction direction;            // 当前方向
    Direction nextDirection;        // 下一个方向
    bool gameOver;                  // 游戏是否结束
    int score;                      // 得分
    bool gameBoard[HEIGHT][WIDTH];  // 游戏边界
    static const int MIN_FOODS = 3; // 最少食物数量
    static const int MAX_FOODS = 7; // 最多食物数量
    bool hintShown;                 // 是否已显示技能提示
    clock_t lastUpdateTime;         // 上次更新时间

public:
    Role role;                      // 角色
    int energy;                     // 能量
    int reviveCount;                // 剩余免死次数

    SnakeGame(Role selectedRole = NORMAL_ROLE) : direction(RIGHT), nextDirection(RIGHT), gameOver(false), score(0), role(selectedRole), energy(0), reviveCount(0), hintShown(false) {
        // 初始化蛇
        snake.push_back(Point(WIDTH / 2, HEIGHT / 2));
        snake.push_back(Point(WIDTH / 2 - 1, HEIGHT / 2));
        snake.push_back(Point(WIDTH / 2 - 2, HEIGHT / 2));
        
        // 随机生成初始食物
        int initialFoods = rand() % (MAX_FOODS - MIN_FOODS + 1) + MIN_FOODS;
        for (int i = 0; i < initialFoods; i++) {
            generateFood();
        }
        
        lastUpdateTime = clock();
    }

    // 生成食物
    void generateFood() {
        Point newFood;
        bool validPosition;
        
        do {
            validPosition = true;
            newFood.x = rand() % (WIDTH - 2) + 1;
            newFood.y = rand() % (HEIGHT - 2) + 1;
            
            // 检查食物是否与蛇身重合
            for (const Point& p : snake) {
                if (p == newFood) {
                    validPosition = false;
                    break;
                }
            }
            
            // 检查食物是否与已有食物重合
            for (const Food& f : foods) {
                if (f.position == newFood) {
                    validPosition = false;
                    break;
                }
            }
        } while (!validPosition);
        
        foods.push_back(Food(newFood));
    }
    
    // 获取食物存在时间（秒）
    double getFoodLifeTime(const Food& food) const {
        return (double)(clock() - food.spawnTime) / CLOCKS_PER_SEC;
    }
    
    // 维持食物数量
    void maintainFoods() {
        // 保证食物数量在 [MIN_FOODS, MAX_FOODS] 范围内
        if (foods.size() < MIN_FOODS) {
            int foodsToAdd = MIN_FOODS - foods.size();
            for (int i = 0; i < foodsToAdd; i++) generateFood();
        } else {
            // 偶尔增加（最多到 MAX_FOODS）
            if (foods.size() < MAX_FOODS && (rand() % 100) < 30) {
                generateFood();
            }
            
            // 移除存在时间超过8秒的食物（但不低于 MIN_FOODS）
            if (foods.size() > MIN_FOODS) {
                // 首先收集可以移除的食物（存在时间超过8秒）
                vector<size_t> oldFoodIndices;
                for (size_t i = 0; i < foods.size(); i++) {
                    if (getFoodLifeTime(foods[i]) >= 8.0) {
                        oldFoodIndices.push_back(i);
                    }
                }
                
                // 如果有超过8秒的食物，随机移除一个（但不低于最小数量）
                if (!oldFoodIndices.empty() && foods.size() > MIN_FOODS) {
                    size_t idxToRemove = oldFoodIndices[rand() % oldFoodIndices.size()];
                    foods.erase(foods.begin() + idxToRemove);
                }
            }
        }
    }

    // 更新游戏状态
    void update() {
        if (gameOver) return;
        
        direction = nextDirection;
        
        // 计算新的头部位置
        Point newHead = snake[0];
        
        switch (direction) {
            case UP:    newHead.y--; break;
            case DOWN:  newHead.y++; break;
            case LEFT:  newHead.x--; break;
            case RIGHT: newHead.x++; break;
            case STOP:  break;
        }
        
        // 检查碰撞（边界）
        if (newHead.x <= 0 || newHead.x >= WIDTH - 1 || 
            newHead.y <= 0 || newHead.y >= HEIGHT - 1) {
            // 若是复苏角色且有复活次数，则消耗一次复活并允许玩家改变方向
            if (role == REVIVE_ROLE && reviveCount > 0) {
                reviveCount--;
                cout << "\n触发边界碰撞，使用一次免死次数。请按方向键选择逃脱方向...\n";
                // 等待玩家按方向键并设置 nextDirection
                while (true) {
                    int k = _getch();
                    if (k == 224) k = _getch();
                    if (k == 72 && direction != DOWN) { nextDirection = UP; break; }
                    if (k == 80 && direction != UP) { nextDirection = DOWN; break; }
                    if (k == 75 && direction != RIGHT) { nextDirection = LEFT; break; }
                    if (k == 77 && direction != LEFT) { nextDirection = RIGHT; break; }
                    // WSAD 支持
                    if (k == 'w' || k == 'W') { if (direction != DOWN) { nextDirection = UP; break; } }
                    if (k == 's' || k == 'S') { if (direction != UP) { nextDirection = DOWN; break; } }
                    if (k == 'a' || k == 'A') { if (direction != RIGHT) { nextDirection = LEFT; break; } }
                    if (k == 'd' || k == 'D') { if (direction != LEFT) { nextDirection = RIGHT; break; } }
                }
                // 重新计算新的头部位置
                newHead = snake[0];
                switch (nextDirection) {
                    case UP:    newHead.y--; break;
                    case DOWN:  newHead.y++; break;
                    case LEFT:  newHead.x--; break;
                    case RIGHT: newHead.x++; break;
                    default: break;
                }
                // 如果新的方向仍然导致碰撞，则判定游戏结束
                if (newHead.x <= 0 || newHead.x >= WIDTH - 1 || newHead.y <= 0 || newHead.y >= HEIGHT - 1) {
                    gameOver = true; return;
                }
            } else {
                gameOver = true; return;
            }
        }
        
        // 检查碰撞（自身）
        for (const Point& p : snake) {
            if (newHead == p) {
                if (role == REVIVE_ROLE && reviveCount > 0) {
                    reviveCount--;
                    cout << "\n触发自身碰撞，使用一次免死次数。请按方向键选择逃脱方向...\n";
                    while (true) {
                        int k = _getch();
                        if (k == 224) k = _getch();
                        if (k == 72 && direction != DOWN) { nextDirection = UP; break; }
                        if (k == 80 && direction != UP) { nextDirection = DOWN; break; }
                        if (k == 75 && direction != RIGHT) { nextDirection = LEFT; break; }
                        if (k == 77 && direction != LEFT) { nextDirection = RIGHT; break; }
                        if (k == 'w' || k == 'W') { if (direction != DOWN) { nextDirection = UP; break; } }
                        if (k == 's' || k == 'S') { if (direction != UP) { nextDirection = DOWN; break; } }
                        if (k == 'a' || k == 'A') { if (direction != RIGHT) { nextDirection = LEFT; break; } }
                        if (k == 'd' || k == 'D') { if (direction != LEFT) { nextDirection = RIGHT; break; } }
                    }
                    // 重新计算新的头部位置
                    newHead = snake[0];
                    switch (nextDirection) {
                        case UP:    newHead.y--; break;
                        case DOWN:  newHead.y++; break;
                        case LEFT:  newHead.x--; break;
                        case RIGHT: newHead.x++; break;
                        default: break;
                    }
                    // 如果仍然与自身重合，则游戏结束
                    bool stillCollision = false;
                    for (const Point& q : snake) if (newHead == q) { stillCollision = true; break; }
                    if (stillCollision) { gameOver = true; return; }
                    // 否则中止碰撞处理，继续游戏
                    break;
                } else {
                    gameOver = true; return;
                }
            }
        }
        
        // 移动蛇
        snake.insert(snake.begin(), newHead);
        
        // 检查是否吃到食物
        bool foodEaten = false;
        for (size_t i = 0; i < foods.size(); i++) {
            if (newHead == foods[i].position) {
                score += 10;
                // 对于清屏/复苏角色，吃到食物回复一点能量
                if (role == CLEAR_ROLE || role == REVIVE_ROLE) energy++;
                foods.erase(foods.begin() + i);
                foodEaten = true;
                break;
            }
        }
        
        if (!foodEaten) {
            snake.pop_back();  // 删除尾部
        }
        
        // 维持食物数量
        maintainFoods();
        
        lastUpdateTime = clock();
    }

    // 天基光束：清除场上所有食物（不回复能量），每个食物 +10 分并增加长度
    void clearSkill() {
        int cnt = (int)foods.size();
        if (cnt == 0) return;
        score += 10 * cnt;
        // 增加长度：复制尾部 cnt 次
        for (int i = 0; i < cnt; i++) {
            snake.push_back(snake.back());
        }
        foods.clear();
        // 维持食物数量会在下一次 update 被触发时补充
    }

    // 再来三局：授予三次免死，每次碰撞（边界或自身）时消耗一次免死机会，允许玩家选择逃脱方向，上限为3次
    void activateReviveSkill() {
        reviveCount = 3;
    }

    // 处理键盘输入
    void handleInput() {
        if (_kbhit()) {
            int key = _getch();
            
                // 处理箭头键（Windows特殊键）
                if (key == 224) {  // 特殊键标记符
                    key = _getch();  // 读取实际的方向键码
                    switch (key) {
                        case 72:  // 上箭头
                            if (direction != DOWN) nextDirection = UP;
                            break;
                        case 80:  // 下箭头
                            if (direction != UP) nextDirection = DOWN;
                            break;
                        case 75:  // 左箭头
                            if (direction != RIGHT) nextDirection = LEFT;
                            break;
                        case 77:  // 右箭头
                            if (direction != LEFT) nextDirection = RIGHT;
                            break;
                    }
                } else {
                    // 技能键：K 或 k
                    if (key == 'k' || key == 'K') {
                        if (role == CLEAR_ROLE && energy >= 10) {
                            energy -= 10;
                            clearSkill();
                        } else if (role == REVIVE_ROLE && energy >= 10) {
                            energy -= 10;
                            activateReviveSkill();
                        }
                    }
                    // 处理WSAD键和退出
                    switch (key) {
                        case 'w':
                        case 'W':
                            if (direction != DOWN) nextDirection = UP;
                            break;
                        case 's':
                        case 'S':
                            if (direction != UP) nextDirection = DOWN;
                            break;
                        case 'a':
                        case 'A':
                            if (direction != RIGHT) nextDirection = LEFT;
                            break;
                        case 'd':
                        case 'D':
                            if (direction != LEFT) nextDirection = RIGHT;
                            break;
                        case 27:  // ESC键退出
                            gameOver = true;
                            break;
                    }
                }
        }
    }

    // 绘制游戏
    void draw() {
        SetCursorPosition(0, 0);  // 移动光标到左上角，而不是清屏
        
        // 创建游戏板
        char board[HEIGHT][WIDTH];
        
        // 初始化边界和空地
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1) {
                    board[y][x] = '#';
                } else {
                    board[y][x] = ' ';
                }
            }
        }
        
        // 绘制所有食物（显示存在时间）
        for (const Food& food : foods) {
            board[food.position.y][food.position.x] = '*';
        }
        
        // 绘制蛇头
        board[snake[0].y][snake[0].x] = '@';
        
        // 绘制蛇身
        for (size_t i = 1; i < snake.size(); i++) {
            board[snake[i].y][snake[i].x] = 'o';
        }
        
        // 输出游戏板
        cout << "\n";
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                cout << board[y][x];
            }
            cout << "\n";
        }
        
        // 输出游戏信息
        cout << "\n得分: " << score << endl;
        // 构造并输出状态行（用空格填充以覆盖上帧残留字符）
        string status = string("蛇长: ") + to_string((int)snake.size()) + string(" | 食物数: ") + to_string((int)foods.size());
        cout << status << string(max(0, 60 - (int)status.size()), ' ') << endl;

        // 显示食物存在时间（可选，用于调试）
        if (!foods.empty()) {
            double minLifeTime = 8.0; // 初始化一个较大的值
            for (const Food& food : foods) {
                double lifeTime = getFoodLifeTime(food);
                if (lifeTime < minLifeTime) minLifeTime = lifeTime;
            }
            string timeInfo = string("最旧食物存在时间: ") + to_string((int)minLifeTime) + "秒";
            cout << timeInfo << string(max(0, 60 - (int)timeInfo.size()), ' ') << endl;
        }

        // 角色与能量行，构造为固定长度以避免残留字符
        string roleLine = string("角色: ");
        switch (role) {
            case NORMAL_ROLE: roleLine += "贪吃蛇"; break;
            case CLEAR_ROLE: roleLine += "炼狱"; break;
            case REVIVE_ROLE: roleLine += "不死鸟"; break;
        }
        roleLine += string(" | 能量: ") + to_string(energy);
        if (role == REVIVE_ROLE) roleLine += string(" | 免死剩余: ") + to_string(reviveCount);
        cout << roleLine << string(max(0, 60 - (int)roleLine.size()), ' ') << endl;

        // 技能行：放在角色行之后，每帧都显示（固定宽度填充），以确保可见
            string skillLine;
            if (role == NORMAL_ROLE) {
                skillLine = "技能: 无";
            } else if (role == CLEAR_ROLE) {
                skillLine = "技能(天基光束): 按K消耗10能量，清除场中所有食物";
            } else if (role == REVIVE_ROLE) {
                skillLine = "技能(再来三局): 按K消耗10能量，获得3次免死机会";
            }
            cout << skillLine << string(max(0, 60 - (int)skillLine.size()), ' ') << endl;

        // 控制行：始终显示，放在技能行之后V
        string controlLine = "控制: W(上) S(下) A(左) D(右) 或 方向键 | ESC(退出)";
        cout << controlLine << string(max(0, 60 - (int)controlLine.size()), ' ') << endl;
        
        if (gameOver) {
            cout << "\n========== 游戏结束 ==========\n";
            cout << "最终得分: " << score << endl;
            cout << "蛇的长度: " << snake.size() << endl;
        }
    }

    // 计算当前游戏速度
    int getFrameRate() const {
        // 初始速度100ms，每增加10分减少5ms，最快30ms
        int baseSpeed = 100;
        int speedIncrease = score / 10 * 5;
        int frameRate = baseSpeed - speedIncrease;
        return frameRate < 30 ? 30 : frameRate;
    }

    // 运行游戏
    void run() {
        while (!gameOver) {
            draw();
            handleInput();
            update();
            int frameRate = getFrameRate();  // 动态调整速度
            Sleep(frameRate);
        }
        
        draw();
    }

    // 获取当前得分
    int getScore() const {
        return score;
    }

    // 获取游戏是否结束
    bool isGameOver() const {
        return gameOver;
    }
};

// 游戏结束菜单（返回选择: 1-重新开始, 2-商店, 3-退出）
int showEndGameMenu(int currentScore, int& highScore, int& points) {
    system("cls");
    cout << "\n" << string(15, '=') << " 游戏结束 " << string(15, '=') << "\n\n";
    cout << "本局得分: " << currentScore << endl;
    
    // 更新历史最高分
    if (currentScore > highScore) {
        highScore = currentScore;
        cout << "【新的最高纪录!】 最高得分: " << highScore << endl;
    } else {
        cout << "历史最高得分: " << highScore << endl;
    }

    // 将得分转化为积分（1分 = 1积分）
    if (currentScore > 0) {
        points += currentScore;
        cout << "本局得分已转化为积分: +" << currentScore << " 积分\n";
        cout << "当前总积分: " << points << "\n";
    }
    
    cout << "\n" << string(40, '-') << "\n";
    cout << "请选择:\n";
    cout << "  1 - 重新开始\n";
    cout << "  2 - 商店\n";
    cout << "  3 - 退出游戏\n";
    cout << "请输入 (1-3): ";

    int choice = 0;
    while (choice < 1 || choice > 3) {
        int input = _getch();
        choice = input - '0';
        if (choice < 1 || choice > 3) {
            cout << "\n输入无效，请重新输入 (1-3): ";
        }
    }
    cout << "\n";
    return choice;
}

// 商店界面：返回后主循环根据选择处理
void showShop(int& points, bool& unlockedClear, bool& unlockedRevive) {
    const int COST = 500;
    while (true) {
        system("cls");
        cout << "\n" << string(12, '=') << " 商店 " << string(12, '=') << "\n\n";
        cout << "当前积分: " << points << "\n\n";
        cout << "可购买角色:\n";
        cout << "  1 - 炼狱     ";
        if (unlockedClear) cout << "[已解锁]\n"; else cout << "[价格: " << COST << " 积分]\n";
        cout << "  2 - 不死鸟   ";
        if (unlockedRevive) cout << "[已解锁]\n"; else cout << "[价格: " << COST << " 积分]\n";
        cout << "  3 - 返回上级菜单\n";
        cout << "请输入选择 (1-3): ";

        int ch = 0;
        while (ch < 1 || ch > 3) {
            int input = _getch();
            ch = input - '0';
        }

        if (ch == 3) return;

        if (ch == 1) {
            if (unlockedClear) {
                cout << "\n炼狱已解锁。按任意键继续..."; _getch();
            } else if (points >= COST) {
                points -= COST;
                unlockedClear = true;
                cout << "\n已购买炼狱！按任意键继续..."; _getch();
            } else {
                cout << "\n积分不足，无法购买。按任意键继续..."; _getch();
            }
        } else if (ch == 2) {
            if (unlockedRevive) {
                cout << "\n不死鸟已解锁。按任意键继续..."; _getch();
            } else if (points >= COST) {
                points -= COST;
                unlockedRevive = true;
                cout << "\n已购买不死鸟！按任意键继续..."; _getch();
            } else {
                cout << "\n积分不足，无法购买。按任意键继续..."; _getch();
            }
        }
    }
}

// 角色选择页面
Role chooseRole(int& points, bool& unlockedClear, bool& unlockedRevive) {
    int roleChoice = 0;
    while (true) {
        system("cls");
        cout << "请选择角色:\n";
        cout << "  1 - 贪吃蛇（无特殊技能）\n";
        cout << "  2 - 炼狱（天基光束：消耗10能量清除场中所有食物) ";
        if (!unlockedClear) cout << "[未解锁]";
        else cout << "[已解锁]";
        cout << "\n";
        cout << "  3 - 不死鸟（再来三局：消耗10能量获得3次免死机会) ";
        if (!unlockedRevive) cout << "[未解锁]";
        else cout << "[已解锁]";
        cout << "\n";
        cout << "  4 - 商店\n";
        cout << "当前积分: " << points << "\n";
        cout << "请输入 (1-4): ";

        int c = _getch();
        if (c >= '1' && c <= '4') roleChoice = c - '0';

        if (roleChoice == 4) {
            showShop(points, unlockedClear, unlockedRevive);
            continue; // 返回角色选择界面
        }

        // 验证是否可以选择
        if (roleChoice == 2 && !unlockedClear) {
            cout << "\n炼狱尚未解锁，无法选择。按任意键返回角色菜单..."; _getch(); continue;
        }
        if (roleChoice == 3 && !unlockedRevive) {
            cout << "\n不死鸟尚未解锁，无法选择。按任意键返回角色菜单..."; _getch(); continue;
        }
        if (roleChoice >=1 && roleChoice <=3) break;
    }

    if (roleChoice == 2) return CLEAR_ROLE;
    if (roleChoice == 3) return REVIVE_ROLE;
    return NORMAL_ROLE;
}

// 主函数
int main() {
    srand((unsigned)time(0));
    
    HideCursor();  // 隐藏光标
    
    cout << "\n" << string(15, '=') << " 瓦罗兰蛇 " << string(15, '=') << "\n";
    cout << "规则: 吃食物增加长度，获得分数，使用分数解锁英雄\n";
    cout << "     不要碰撞自身和边界\n";
    cout << "控制: W(上) S(下) A(左) D(右) 或 方向键 | ESC(退出)\n";
    cout << "技能: 按K键使用\n";
    system("pause");
    
    int highScore = 0;  // 记录历史最高分
    bool continueGame = true;
    int points = 0;                  // 玩家积分（可用于解锁角色）
    bool unlockedClear = false;      // 炼狱是否已解锁
    bool unlockedRevive = false;     // 不死鸟是否已解锁

    while (continueGame) {
        system("cls");  // 清屏，避免之前的内容与游戏板重叠

        // 角色选择（调用独立函数 chooseRole）
        Role selectedRole = chooseRole(points, unlockedClear, unlockedRevive);
        system("cls"); // 清理角色选择界面残留

        SnakeGame game(selectedRole);
        game.run();

        // 游戏结束后将得分转为积分并显示带商店选项
        int menuChoice = showEndGameMenu(game.getScore(), highScore, points);
        if (menuChoice == 1) {
            continueGame = true; // 重新开始
            continue;
        } else if (menuChoice == 2) {
            // 进入商店
            showShop(points, unlockedClear, unlockedRevive);
            // 商店后回到主循环（角色选择）
            continueGame = true;
            continue;
        } else {
            // 退出游戏
            continueGame = false;
            break;
        }
    }
    
    system("cls");
    ShowCursor();  // 显示光标
    cout << "\n感谢游玩！\n";
    cout << "最高得分: " << highScore << " 分\n";
    cout << "最终积分: " << points << " 积分\n\n";
    system("pause");
    
    return 0;
}