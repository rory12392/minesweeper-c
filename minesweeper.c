#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h> 
#include <string.h> 
#include <windows.h>


// --- 常數定義 ---
#define MAX_ROWS 30
#define MAX_COLS 30
#define DEFAULT_ROWS 9
#define DEFAULT_COLS 9
#define DEFAULT_MINES 10
#define PLAYER_LIVES 3 // 預設生命值
#define RADAR_COUNT 2 // 雷達格數量
#define RADAR_RANGE 2 // 雷達偵測半徑 (2 表示以中心為軸，左右上下各延伸 2 格，總共 5x5)

#define SAVE_FILE_NAME "minesweeper_save.dat" // 遊戲存檔檔名
#define RANKING_FILE_NAME "minesweeper_ranking.txt" // 排行榜檔名
#define MAX_RANKING_ENTRIES 10 // 排行榜最大條目數
#define MAX_NAME_LENGTH 20 // 玩家姓名最大長度

// ANSI Escape Codes for colors and text styles (僅限支援 ANSI 的終端機)
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define BG_GRAY "\033[47m" // 白色背景，用於未翻開的格子
#define BG_DEFAULT "\033[49m"
#define COLOR_BLACK "\033[30m" // 黑色字體，用於 BG_GRAY 上的 8

// 地雷格子的結構
typedef struct {
    int isMine;         // 是否為地雷
    int isRevealed;     // 是否已翻開
    int isFlagged;      // 是否已標記旗幟
    int adjacentMines;  // 周圍地雷數
    int isRadar;        // 是否為雷達格
} Cell;

// 遊戲存檔結構體
typedef struct {
    int rows;
    int cols;
    int totalMines;
    int currentLives;
    time_t savedTimeOffset; // 遊戲已進行的時間 (從開始到存檔的時間差)
    Cell cells[MAX_ROWS][MAX_COLS]; // 直接嵌入盤面數據
} GameSave;

// --- 排行榜條目結構體 ---
typedef struct {
    char name[MAX_NAME_LENGTH + 1]; // 玩家姓名
    long timeTaken;                  // 遊戲時間 (秒)
    int rows;                        // 盤面行數
    int cols;                        // 盤面列數
    int mines;                       // 地雷數
} RankingEntry;

// --- 全局變數 ---
time_t startTime;
time_t endTime;
int currentLives; // 當前生命值
int totalMines;   // 記錄當前遊戲的地雷總數
int gameRows, gameCols; // 記錄當前遊戲的行數和列數 (用於存檔和排行榜)

// --- 計算旗幟數量 ---
int countFlags(Cell** board, int rows, int cols) {
    int count = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (board[i][j].isFlagged) {
                count++;
            }
        }
    }
    return count;
}

// --- 函數原型宣告 ---
void clearScreen();
void printWelcomeMessage();
void displayBoard(Cell** board, int rows, int cols, int showAllMines);
void revealAllMines(Cell** board, int rows, int cols);
void revealCellRecursive(Cell** board, int rows, int cols, int row, int col);
void activateRadar(Cell** board, int rows, int cols, int row, int col);
void revealCell(Cell** board, int rows, int cols, int row, int col);
void flagCell(Cell** board, int rows, int cols, int row, int col);
int revealAroundIfFlagsMatch(Cell** board, int rows, int cols, int row, int col);
int checkWin(Cell** board, int rows, int cols);
Cell** initializeBoard(int rows, int cols);
void placeMines(Cell** board, int rows, int cols, int mineCount, int firstClickRow, int firstClickCol);
void placeRadarCells(Cell** board, int rows, int cols, int radarCount, int firstClickRow, int firstClickCol);
void calculateAdjacentMines(Cell** board, int rows, int cols);
void freeBoard(Cell** board, int rows);
int gameLoop(Cell** board, int rows, int cols, int mineCount);
void selectDifficulty(int *rows, int *cols, int *mineCount);
int getValidIntegerInput(const char* prompt, int min, int max);
void playAgainPrompt(int *playAgain);

// --- 存檔/讀檔函數原型 ---
int saveGame(Cell** board, int rows, int cols);
Cell** loadGame(int *rows, int *cols, int *mineCount);

// --- 排行榜函數原型 ---
void displayRanking();
void addRankingEntry(const char* name, long timeTaken, int rows, int cols, int mines);
void sortRanking(RankingEntry ranking[], int count);
void saveRanking(RankingEntry ranking[], int count);
int loadRanking(RankingEntry ranking[]);


// 遊戲互動與視覺效果相關函數 

// 清除終端機螢幕
void clearScreen() {
    system("cls");
}

// 歡迎訊息
void printWelcomeMessage() {
    clearScreen();
    printf(COLOR_CYAN "=======================================\n");
    printf("         歡迎來到踩地雷！\n");
    printf("=======================================\n" COLOR_RESET);
    printf("目標：翻開所有非地雷的格子。\n");
    printf("操作：\n");
    printf("  'R' 或 'r'：翻開格子\n");
    printf("  'F' 或 'f'：標記/取消標記旗幟\n");
    printf("  'C' 或 'c'：翻開周圍（如果旗幟數匹配）\n");
    printf("  'S' 或 's'：存檔\n"); // 新增存檔提示
    printf("  'Q' 或 'q'：離開遊戲\n"); // 新增離開提示
    printf("---------------------------------------\n");
}

// 顯示遊戲盤面狀態
void displayBoard(Cell** board, int rows, int cols, int showAllMines) {
    clearScreen();
    printWelcomeMessage();

    printf("  ");
    for (int j = 0; j < cols; j++) {
        printf("%2d ", j);
    }
    printf("\n");
    printf("--");
    for (int j = 0; j < cols; j++) {
        printf("---");
    }
    printf("\n");

    for (int i = 0; i < rows; i++) {
        printf("%2d|", i);
        for (int j = 0; j < cols; j++) {
            if (board[i][j].isFlagged) {
                printf(COLOR_YELLOW " F " COLOR_RESET);
            } else if (board[i][j].isRevealed) {
                if (board[i][j].isMine) {
                    printf(COLOR_RED " * " COLOR_RESET);
                } else if (board[i][j].isRadar) {
                    printf(COLOR_MAGENTA " R " COLOR_RESET);
                } else {
                    switch (board[i][j].adjacentMines) {
                        case 0: printf(BG_DEFAULT "   " COLOR_RESET); break;
                        case 1: printf(COLOR_BLUE " 1 " COLOR_RESET); break;
                        case 2: printf(COLOR_GREEN " 2 " COLOR_RESET); break;
                        case 3: printf(COLOR_RED " 3 " COLOR_RESET); break;
                        case 4: printf(COLOR_MAGENTA " 4 " COLOR_RESET); break;
                        case 5: printf(COLOR_YELLOW " 5 " COLOR_RESET); break;
                        case 6: printf(COLOR_CYAN " 6 " COLOR_RESET); break;
                        case 7: printf(COLOR_WHITE " 7 " COLOR_RESET); break;
                        case 8: printf(BG_GRAY COLOR_BLACK " 8 " COLOR_RESET); break;
                        default: printf(" %d ", board[i][j].adjacentMines); break;
                    }
                }
            } else {
                if (showAllMines && board[i][j].isMine) {
                    printf(COLOR_RED " * " COLOR_RESET);
                } else {
                    printf(BG_GRAY " X " COLOR_RESET);
                }
            }
        }
        printf("\n");
    }
    // 顯示生命值、旗幟數與經過時間
    printf("\n生命值: %d\n", currentLives);
    printf("旗幟數: %d / %d\n", countFlags(board, rows, cols), totalMines);
    if (startTime != 0) {
        printf("經過時間: %ld 秒\n", (long)(time(NULL) - startTime));
    }
}

// 顯示所有地雷 (遊戲結束時用)
void revealAllMines(Cell** board, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (board[i][j].isMine) {
                board[i][j].isRevealed = 1;
            }
        }
    }
    displayBoard(board, rows, cols, 1); // 傳入 1 讓 displayBoard 顯示所有地雷
}


// 初始化遊戲盤面
Cell** initializeBoard(int rows, int cols) {
    Cell** board = (Cell**)malloc(rows * sizeof(Cell*));
    if (board == NULL) {
        perror("Failed to allocate memory for board rows");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < rows; i++) {
        board[i] = (Cell*)malloc(cols * sizeof(Cell));
        if (board[i] == NULL) {
            perror("Failed to allocate memory for board columns");
            for (int k = 0; k < i; k++) {
                free(board[k]);
            }
            free(board);
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < cols; j++) {
            board[i][j].isMine = 0;
            board[i][j].isRevealed = 0;
            board[i][j].isFlagged = 0;
            board[i][j].adjacentMines = 0;
            board[i][j].isRadar = 0; // 初始化雷達格為否
        }
    }
    return board;
}

// 隨機布置地雷 (新增 firstClickRow, firstClickCol 參數，用於安全開局)
void placeMines(Cell** board, int rows, int cols, int mineCount, int firstClickRow, int firstClickCol) {
    srand(time(NULL));
    int placedMines = 0;
    while (placedMines < mineCount) {
        int row = rand() % rows;
        int col = rand() % cols;

        // 確保地雷不會放置在第一次點擊的格子或其周圍 (3x3 安全區)
        if (row >= firstClickRow -1 && row <= firstClickRow + 1 &&
            col >= firstClickCol - 1 && col <= firstClickCol + 1) {
            continue;
        }
        // 確保不與雷達格重疊
        if (board[row][col].isMine == 0 && board[row][col].isRadar == 0) {
            board[row][col].isMine = 1;
            placedMines++;
        }
    }
}

// 隨機布置雷達格
void placeRadarCells(Cell** board, int rows, int cols, int radarCount, int firstClickRow, int firstClickCol) {
    srand(time(NULL));
    int placedRadar = 0;
    while (placedRadar < radarCount) {
        int row = rand() % rows;
        int col = rand() % cols;

        // 確保雷達格不會放置在第一次點擊的格子或其周圍 (3x3 安全區)
        if (row >= firstClickRow -1 && row <= firstClickRow + 1 &&
            col >= firstClickCol - 1 && col <= firstClickCol + 1) {
            continue;
        }
        // 確保不與地雷或其他雷達格重疊
        if (board[row][col].isMine == 0 && board[row][col].isRadar == 0) {
            board[row][col].isRadar = 1;
            placedRadar++;
        }
    }
}

// 計算周圍地雷數
void calculateAdjacentMines(Cell** board, int rows, int cols) {
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (board[i][j].isMine == 1) continue;
            if (board[i][j].isRadar == 1) continue; // 雷達格不需要計算周圍地雷數

            int mineCount = 0;
            for (int k = 0; k < 8; k++) {
                int newRow = i + dx[k];
                int newCol = j + dy[k];
                if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
                    if (board[newRow][newCol].isMine == 1) {
                        mineCount++;
                    }
                }
            }
            board[i][j].adjacentMines = mineCount;
        }
    }
}

// 遞迴翻開格子 (沒有地雷且周圍地雷數為 0 時)
void revealCellRecursive(Cell** board, int rows, int cols, int row, int col) {
    if (row < 0 || row >= rows || col < 0 || col >= cols || board[row][col].isRevealed || board[row][col].isFlagged) {
        return;
    }

    board[row][col].isRevealed = 1;

    // 只有當當前格子周圍沒有地雷時，才繼續遞迴翻開周圍格子
    if (board[row][col].adjacentMines == 0 && board[row][col].isRadar == 0) {
        int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
        int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int i = 0; i < 8; i++) {
            revealCellRecursive(board, rows, cols, row + dx[i], col + dy[i]);
        }
    }
}

// 啟動雷達格功能
void activateRadar(Cell** board, int rows, int cols, int row, int col) {
    board[row][col].isRevealed = 1;
    printf(COLOR_MAGENTA "雷達啟動！掃描周圍地雷並標記！\n" COLOR_RESET);
    // 掃描以 (row, col) 為中心的 5x5 區域 (半徑為 RADAR_RANGE)
    for (int i = row - RADAR_RANGE; i <= row + RADAR_RANGE; i++) {
        for (int j = col - RADAR_RANGE; j <= col + RADAR_RANGE; j++) {
            if (i >= 0 && i < rows && j >= 0 && j < cols) {
                // 如果是地雷且尚未被翻開或標記，則自動標記旗幟
                if (board[i][j].isMine && !board[i][j].isRevealed && !board[i][j].isFlagged) {
                    board[i][j].isFlagged = 1; // 自動標記旗幟
                    printf(COLOR_GREEN "雷達在 (%d, %d) 發現並標記地雷！\n" COLOR_RESET, i, j);
                }
            }
        }
    }
}

// 翻開格子
void revealCell(Cell** board, int rows, int cols, int row, int col) {
    if (row < 0 || row >= rows || col < 0 || col >= cols || board[row][col].isRevealed || board[row][col].isFlagged) {
        return; // 無效操作
    }

    // 檢查是否是第一次點擊，如果是，則開始計時並佈雷和雷達格
    // 注意：如果遊戲是從存檔載入的，startTime可能不為0，但地雷和雷達格已經佈置
    if (startTime == 0) {
        startTime = time(NULL);
        placeMines(board, rows, cols, totalMines, row, col); // 第一次點擊後再佈雷
        placeRadarCells(board, rows, cols, RADAR_COUNT, row, col); // 第一次點擊後佈置雷達格
        calculateAdjacentMines(board, rows, cols); // 重新計算地雷數
    }

    board[row][col].isRevealed = 1;

    if (board[row][col].isMine) {
        currentLives--; // 生命值減一
        printf(COLOR_RED "你踩到地雷了！生命值 -1。剩餘生命值: %d\n" COLOR_RESET, currentLives);
        Sleep(2000);
        // 如果生命值歸零，則遊戲結束
        if (currentLives <= 0) {
            printf(COLOR_RED "生命值歸零！遊戲結束！\n" COLOR_RESET);
            return; // 返回，在 gameLoop 中處理遊戲結束
        }
        // 如果還有生命，只是扣血，格子仍顯示為地雷，但遊戲繼續
        return;
    }

    if (board[row][col].isRadar) { // 如果翻開的是雷達格
        activateRadar(board, rows, cols, row, col);
        return; // 雷達格被翻開後不繼續遞迴翻開其他格子
    }

    // 如果周圍地雷數為 0，則遞迴翻開周圍格子
    if (board[row][col].adjacentMines == 0) {
        revealCellRecursive(board, rows, cols, row, col);
    }
}

// 標記旗幟
void flagCell(Cell** board, int rows, int cols, int row, int col) {
    if (row < 0 || row >= rows || col < 0 || col >= cols || board[row][col].isRevealed) {
        return;
    }
    board[row][col].isFlagged = !board[row][col].isFlagged;
}

// 新增功能：如果數字格周圍的旗幟數量與其數字匹配，則翻開周圍未翻開且未標記的格子
// 返回值：0 表示沒有觸發或觸發失敗（踩到雷），1 表示成功翻開周圍格子
int revealAroundIfFlagsMatch(Cell** board, int rows, int cols, int row, int col) {
    // 1. 檢查點擊的格子是否已翻開且不是地雷或雷達格 (必須是數字格)
    if (!board[row][col].isRevealed || board[row][col].isMine || board[row][col].isRadar) {
        printf(COLOR_YELLOW "這個格子無法自動翻開周圍（必須是已翻開的數字格）。\n" COLOR_RESET);
        Sleep(1500);
        return 0;
    }

    int adjacentFlags = 0;
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    // 2. 計算周圍已標記的旗幟數量
    for (int k = 0; k < 8; k++) {
        int newRow = row + dx[k];
        int newCol = col + dy[k];

        if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
            if (board[newRow][newCol].isFlagged) {
                adjacentFlags++;
            }
        }
    }

    // 3. 檢查周圍旗幟數量是否與當前格子的數字匹配
    if (adjacentFlags == board[row][col].adjacentMines) {
        printf(COLOR_GREEN "周圍旗幟數量匹配！自動翻開周圍格子。\n" COLOR_RESET);
        Sleep(1000);
        // 4. 翻開周圍所有未翻開且未標記的格子
        for (int k = 0; k < 8; k++) {
            int newRow = row + dx[k];
            int newCol = col + dy[k];

            if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
                // 只有未翻開且未標記的格子才會被嘗試翻開
                if (!board[newRow][newCol].isRevealed && !board[newRow][newCol].isFlagged) { // 修正為 newRow, newCol
                     // 這裡直接呼叫 revealCell 來處理可能踩到地雷或連鎖翻開的情況
                    revealCell(board, rows, cols, newRow, newCol); 
                    // 如果踩到地雷導致生命值歸零，則返回 0
                    if (currentLives <= 0) {
                        return 0; 
                    }
                }
            }
        }
        return 1; // 成功觸發
    } else {
        printf(COLOR_YELLOW "周圍旗幟數量不匹配，無法自動翻開周圍格子。\n" COLOR_RESET);
        Sleep(1500);
        return 0; // 未觸發
    }
}


// 檢查勝負條件
int checkWin(Cell** board, int rows, int cols) {
    int revealedNonMines = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // 所有非地雷、非雷達格的格子都已被翻開
            if (board[i][j].isRevealed && !board[i][j].isMine && !board[i][j].isRadar) {
                revealedNonMines++;
            }
        }
    }
    // 勝利條件：所有非地雷、非雷達格的格子都被翻開
    return (revealedNonMines == (rows * cols - totalMines - RADAR_COUNT)); 
}

// 釋放記憶體
void freeBoard(Cell** board, int rows) {
    if (board == NULL) return;
    for (int i = 0; i < rows; i++) {
        if (board[i] != NULL) {
            free(board[i]);
        }
    }
    free(board);
}

// 獲取有效整數輸入
int getValidIntegerInput(const char* prompt, int min, int max) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1) {
            // 清空輸入緩衝區
            while (getchar() != '\n');
            if (value >= min && value <= max) {
                return value;
            } else {
                printf(COLOR_RED "輸入超出範圍，請輸入 %d 到 %d 之間的數字。\n" COLOR_RESET, min, max);
            }
        } else {
            printf(COLOR_RED "無效輸入，請輸入一個整數。\n" COLOR_RESET);
            // 清空輸入緩衝區
            while (getchar() != '\n');
        }
    }
}

// 遊戲難度選擇
void selectDifficulty(int *rows, int *cols, int *mineCount) {
    int choice;
    printf("\n選擇難度:\n");
    printf("1. 初級 (9x9 盤面, 10 個地雷)\n");
    printf("2. 中級 (16x16 盤面, 40 個地雷)\n");
    printf("3. 高級 (16x30 盤面, 99 個地雷)\n");
    printf("4. 自訂\n");

    choice = getValidIntegerInput("請輸入您的選擇 (1-4): ", 1, 4);



    switch (choice) {
        case 1:
            *rows = 9;
            *cols = 9;
            *mineCount = 10;
            break;
        case 2:
            *rows = 16;
            *cols = 16;
            *mineCount = 40;
            break;
        case 3:
            *rows = 16;
            *cols = 30;
            *mineCount = 99;
            break;
        case 4:
            printf("\n--- 自訂難度 ---\n");
            *rows = getValidIntegerInput("輸入列數 (5-30): ", 5, MAX_ROWS);
            *cols = getValidIntegerInput("輸入欄數 (5-30): ", 5, MAX_COLS);
            // 建議地雷數上限為盤面面積的1/3，並確保有足夠的空間放置雷達格
            int maxMines = (*rows * *cols) / 3;
            // 確保地雷數量不會導致無法開局 (至少保留 9 個安全格 + 雷達格數量)
            int minSafeCells = 9 + RADAR_COUNT;
            if (maxMines > (*rows * *cols) - minSafeCells) {
                maxMines = (*rows * *cols) - minSafeCells;
            }
            if (maxMines < 1) maxMines = 1; // 至少可以有一個地雷

            *mineCount = getValidIntegerInput("輸入地雷數量 (1-N): ", 1, maxMines);
            
            // 再次檢查，如果地雷數量太高，自動調整
            if (*mineCount >= (*rows * *cols) - minSafeCells) {
                *mineCount = (*rows * *cols) - minSafeCells;
                printf(COLOR_YELLOW "地雷數量過多，已自動調整為 %d 個，確保可以安全開局。\n" COLOR_RESET, *mineCount);
            }
            break;
    }
    printf(COLOR_GREEN "遊戲設定: %d 列 x %d 欄, %d 個地雷, %d 個雷達格\n" COLOR_RESET, *rows, *cols, *mineCount, RADAR_COUNT);
    totalMines = *mineCount; // 更新全局地雷數
    gameRows = *rows; // 更新全局行數
    gameCols = *cols; // 更新全局列數
}

// 詢問玩家是否再玩一次
void playAgainPrompt(int *playAgain) {
    char choice;
    printf("\n是否再玩一次? (Y/N): ");
    // 確保只讀取一個字元，並忽略剩餘輸入
    while (scanf(" %c", &choice) != 1) {
        printf(COLOR_RED "無效輸入，請輸入 Y 或 N: " COLOR_RESET);
        while (getchar() != '\n'); // 清空緩衝區
    }
    while (getchar() != '\n'); // 清空緩衝區
    
    choice = toupper(choice); // 轉換為大寫

    if (choice == 'Y') {
        *playAgain = 1;
    } else {
        *playAgain = 0;
    }
}

// 儲存遊戲進度
int saveGame(Cell** board, int rows, int cols) {
    FILE *fp;
    GameSave save_data;

    // 填充存檔數據
    save_data.rows = rows;
    save_data.cols = cols;
    save_data.totalMines = totalMines; // 使用全局變數
    save_data.currentLives = currentLives; // 使用全局變數
    save_data.savedTimeOffset = time(NULL) - startTime; // 計算已玩時間

    // 將動態分配的盤面數據複製到靜態數組中
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            save_data.cells[i][j] = board[i][j];
        }
    }

    fp = fopen(SAVE_FILE_NAME, "wb"); // 以二進制寫模式打開
    if (fp == NULL) {
        perror(COLOR_RED "無法開啟存檔檔案！\n" COLOR_RESET);
        Sleep(2000);
        return 0;
    }

    if (fwrite(&save_data, sizeof(GameSave), 1, fp) != 1) {
        perror(COLOR_RED "寫入存檔失敗！\n" COLOR_RESET);
        Sleep(2000);
        fclose(fp);
        return 0;
    }

    fclose(fp);
    printf(COLOR_GREEN "遊戲已存檔到 '%s'。\n" COLOR_RESET, SAVE_FILE_NAME);
    Sleep(2000);
    return 1;
}

// 載入遊戲進度
// 返回值：載入成功的盤面 (Cell**)，失敗則為 NULL
Cell** loadGame(int *rows, int *cols, int *mineCount) {
    FILE *fp;
    GameSave load_data;

    fp = fopen(SAVE_FILE_NAME, "rb"); // 以二進制讀模式打開
    if (fp == NULL) {
        perror(COLOR_RED "無法開啟存檔檔案或存檔不存在。\n" COLOR_RESET);
        Sleep(2000);
        return NULL;
    }

    if (fread(&load_data, sizeof(GameSave), 1, fp) != 1) {
        perror(COLOR_RED "讀取存檔失敗或存檔格式不正確！\n" COLOR_RESET);
        Sleep(2000);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    // 更新全局遊戲狀態
    *rows = load_data.rows;
    *cols = load_data.cols;
    *mineCount = load_data.totalMines; // 更新傳入的地雷數指針
    totalMines = load_data.totalMines; // 更新全局地雷數
    currentLives = load_data.currentLives; // 更新全局生命值
    // 重新計算 startTime，使其基於載入時已玩的時間
    startTime = time(NULL) - load_data.savedTimeOffset; 
    gameRows = *rows; // 更新全局行數
    gameCols = *cols; // 更新全局列數

    // 重新動態分配盤面並複製載入的數據
    Cell** board = initializeBoard(*rows, *cols);
    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            board[i][j] = load_data.cells[i][j];
        }
    }

    printf(COLOR_GREEN "遊戲已從 '%s' 載入！\n" COLOR_RESET, SAVE_FILE_NAME);
    Sleep(2000);
    return board;
}

// 比較函數，用於qsort排序 (時間短的排在前面，時間相同則按盤面大小等)
int compareRankingEntries(const void *a, const void *b) {
    RankingEntry *entryA = (RankingEntry *)a;
    RankingEntry *entryB = (RankingEntry *)b;

    // 首先按時間升序排序
    if (entryA->timeTaken < entryB->timeTaken) return -1;
    if (entryA->timeTaken > entryB->timeTaken) return 1;

    // 時間相同，按盤面大小（總格子數）降序排序（大盤面相同時間更難）
    int sizeA = entryA->rows * entryA->cols;
    int sizeB = entryB->rows * entryB->cols;
    if (sizeA > sizeB) return -1;
    if (sizeA < sizeB) return 1;

    // 盤面大小相同，按地雷數降序排序
    if (entryA->mines > entryB->mines) return -1;
    if (entryA->mines < entryB->mines) return 1;

    return 0; // 完全相同
}

// 載入排行榜數據
int loadRanking(RankingEntry ranking[]) {
    FILE *fp;
    int count = 0;
    fp = fopen(RANKING_FILE_NAME, "r"); // 以讀模式打開
    if (fp == NULL) {
        return 0; // 檔案不存在，返回0個條目
    }

    while (count < MAX_RANKING_ENTRIES && 
           fscanf(fp, "%s %ld %d %d %d\n", 
                  ranking[count].name, 
                  &ranking[count].timeTaken, 
                  &ranking[count].rows, 
                  &ranking[count].cols, 
                  &ranking[count].mines) == 5) {
        count++;
    }
    fclose(fp);
    return count;
}

// 保存排行榜數據
void saveRanking(RankingEntry ranking[], int count) {
    FILE *fp;
    fp = fopen(RANKING_FILE_NAME, "w"); // 以寫模式打開 (會清空舊內容)
    if (fp == NULL) {
        perror(COLOR_RED "無法保存排行榜檔案！\n" COLOR_RESET);
        Sleep(2000);
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s %ld %d %d %d\n", 
                ranking[i].name, 
                ranking[i].timeTaken, 
                ranking[i].rows, 
                ranking[i].cols, 
                ranking[i].mines);
    }
    fclose(fp);
}

// 將新的成績加入排行榜
void addRankingEntry(const char* name, long timeTaken, int rows, int cols, int mines) {
    RankingEntry ranking[MAX_RANKING_ENTRIES];
    int count = loadRanking(ranking);

    // 建立新的條目
    RankingEntry newEntry;
    strncpy(newEntry.name, name, MAX_NAME_LENGTH);
    newEntry.name[MAX_NAME_LENGTH] = '\0'; // 確保字串結尾
    newEntry.timeTaken = timeTaken;
    newEntry.rows = rows;
    newEntry.cols = cols;
    newEntry.mines = mines;

    // 如果排行榜未滿，直接加入
    if (count < MAX_RANKING_ENTRIES) {
        ranking[count] = newEntry;
        count++;
    } else {
        // 如果排行榜已滿，檢查新成績是否比最差的成績好
        // (先排序才能知道最差的是哪一個)
        qsort(ranking, count, sizeof(RankingEntry), compareRankingEntries);
        if (compareRankingEntries(&newEntry, &ranking[MAX_RANKING_ENTRIES - 1]) < 0) {
            ranking[MAX_RANKING_ENTRIES - 1] = newEntry; // 替換掉最差的
        }
    }

    // 重新排序並保存
    qsort(ranking, count, sizeof(RankingEntry), compareRankingEntries);
    saveRanking(ranking, count);
}

// 顯示排行榜
void displayRanking() {
    clearScreen();
    printf(COLOR_CYAN "=======================================\n");
    printf("             踩地雷 排行榜\n");
    printf("=======================================\n" COLOR_RESET);

    RankingEntry ranking[MAX_RANKING_ENTRIES];
    int count = loadRanking(ranking);

    if (count == 0) {
        printf("目前沒有任何排行榜記錄。\n");
    } else {
        printf("No. 姓名                 時間(秒) 盤面\n");
        printf("--- -------------------- -------- --------\n");
        for (int i = 0; i < count; i++) {
            printf("%2d. %-20s %8ld %2dx%-2d(%2d)\n", 
                   i + 1, 
                   ranking[i].name, 
                   ranking[i].timeTaken, 
                   ranking[i].rows, 
                   ranking[i].cols,
                   ranking[i].mines);
        }
    }
    printf("---------------------------------------\n");
    printf("按任意鍵返回主選單...");
    getchar(); // 吸收換行符
    getchar(); // 等待用戶輸入
}

// 遊戲迴圈
// 返回值：0 遊戲正常結束 (輸或贏)，1 玩家選擇退出或存檔
int gameLoop(Cell** board, int rows, int cols, int mineCount) {
    int gameOver = 0;
    char operation_char; // 使用 char 來接收操作指令
    int row, col;
    int gameExit = 0; // 標誌是否是玩家選擇退出或存檔

    // 如果是從存檔載入，startTime已經在loadGame中設置，否則為0
    // 如果是新遊戲，則在第一次點擊時設置startTime

    displayBoard(board, rows, cols, 0);

    while (!gameOver && !gameExit) {
        printf("\n請輸入操作 (R: 翻開, F: 標記/取消標記, C: 清空周圍, S: 存檔, Q: 離開) 及行列號 (例如: R 0 0): ");
        char inputBuffer[50]; // 緩衝區用於讀取整行輸入
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL) {
            printf(COLOR_RED "輸入錯誤，請重新輸入。\n" COLOR_RESET);
            continue;
        }

        // 解析輸入
        int scanned_items = sscanf(inputBuffer, " %c %d %d", &operation_char, &row, &col);
        operation_char = toupper(operation_char);

        if (scanned_items < 1) { // 至少要輸入操作指令
             printf(COLOR_RED "輸入格式錯誤，請重新輸入。\n" COLOR_RESET);
             continue;
        }
        
        // 處理不帶行列號的操作 (S, Q)
        if (operation_char == 'S') {
            if (startTime == 0) {
                printf(COLOR_YELLOW "遊戲尚未開始，無法存檔。\n" COLOR_RESET);
                continue;
            }
            if (saveGame(board, rows, cols)) {
                gameExit = 1; // 存檔後退出遊戲迴圈
            }
            continue;
        }
        if (operation_char == 'Q') {
            printf(COLOR_YELLOW "你確定要離開遊戲嗎？(Y/N): " COLOR_RESET);
            char confirm_char;
            if (scanf(" %c", &confirm_char) == 1) {
                while(getchar() != '\n'); // 清空緩衝區
                if (toupper(confirm_char) == 'Y') {
                    gameExit = 1; // 玩家確認離開
                }
            } else {
                while(getchar() != '\n'); // 清空緩衝區
            }
            if (gameExit) continue; // 如果確定離開，跳過後續處理
            // 如果不確定離開，重新顯示盤面
            displayBoard(board, rows, cols, 0);
            continue;
        }

        // 處理需要行列號的操作 (R, F, C)
        if (scanned_items != 3) {
            printf(COLOR_RED "輸入格式錯誤，請重新輸入 (例如: R 0 0)。\n" COLOR_RESET);
            continue;
        }

        if (row < 0 || row >= rows || col < 0 || col >= cols) {
            printf(COLOR_RED "輸入的行列號超出範圍 (%d-%d, %d-%d)。\n" COLOR_RESET, 0, rows - 1, 0, cols - 1);
            continue;
        }

        if (operation_char == 'R') {
            // 在第一次點擊時才佈置地雷和雷達格，並計算周圍地雷數
            if (startTime == 0) {
                startTime = time(NULL);
                placeMines(board, rows, cols, totalMines, row, col);
                placeRadarCells(board, rows, cols, RADAR_COUNT, row, col);
                calculateAdjacentMines(board, rows, cols);
            }

            // 檢查是否已翻開或已標記
            if (board[row][col].isRevealed || board[row][col].isFlagged) {
                printf(COLOR_YELLOW "該格子已經翻開或已標記旗幟，請選擇其他格子。\n" COLOR_RESET);
                Sleep(2000);
            } else {
                revealCell(board, rows, cols, row, col);
                // 檢查是否因踩到地雷而生命值歸零
                if (board[row][col].isMine && currentLives <= 0) { // 確保確實是踩到地雷且生命值歸零
                    gameOver = 1;
                    printf(COLOR_RED "你已失去所有生命！遊戲結束！\n" COLOR_RESET);
                    endTime = time(NULL);
                    printf("總耗時: %ld 秒\n", (long)(endTime - startTime));
                    Sleep(3000);
                    revealAllMines(board, rows, cols); // 顯示所有地雷
                }
            }
        } else if (operation_char == 'F') {
            flagCell(board, rows, cols, row, col);
        } else if (operation_char == 'C') { // 新增 'C' 操作
            // 如果是第一次點擊，提示無法使用此功能
            if (startTime == 0) {
                printf(COLOR_YELLOW "遊戲尚未開始，無法使用。請先翻開一個格子。\n" COLOR_RESET);
                continue;
            }
            int result = revealAroundIfFlagsMatch(board, rows, cols, row, col);
            // 如果因為自動翻開而踩到雷並生命值歸零
            if (result == 0 && currentLives <= 0) {
                 gameOver = 1;
                 printf(COLOR_RED "你已失去所有生命！遊戲結束！\n" COLOR_RESET);
                 endTime = time(NULL);
                 printf("總耗時: %ld 秒\n", (long)(endTime - startTime));
                 Sleep(3000);
                 revealAllMines(board, rows, cols); // 顯示所有地雷
            }
        }
        else {
            printf(COLOR_RED "無效的操作，請輸入 'R', 'F', 'C', 'S' 或 'Q'。\n" COLOR_RESET);
            continue;
        }

        // 每次操作後顯示盤面 (僅在遊戲未結束且未退出時)
        if (!gameOver && !gameExit) {
            displayBoard(board, rows, cols, 0);
        }

        // 檢查勝利條件 (僅在遊戲尚未結束時檢查)
        if (!gameOver && !gameExit && checkWin(board, rows, cols)) {
            printf(COLOR_GREEN "恭喜你！你贏了！\n" COLOR_RESET);
            endTime = time(NULL);
            long time_taken = (long)(endTime - startTime);
            printf("總耗時: %ld 秒\n", time_taken);

            // 詢問玩家姓名並加入排行榜
            char playerName[MAX_NAME_LENGTH + 1];
            printf("請輸入你的姓名 (最多 %d 個字元): ", MAX_NAME_LENGTH);
            // 使用 fgets 讀取整行，避免空格問題，並清理換行符
            if (fgets(playerName, sizeof(playerName), stdin) != NULL) {
                playerName[strcspn(playerName, "\n")] = 0; // 移除換行符
            } else {
                strcpy(playerName, "Unnamed"); // 讀取失敗給個預設名
            }
            addRankingEntry(playerName, time_taken, rows, cols, mineCount);

            gameOver = 1; // 遊戲勝利結束
        }
    }
    return gameExit; // 返回是否是玩家主動退出
}

int main() {
    int rows, cols, mineCount;
    int playAgain = 1;
    Cell** board = NULL; // 初始化為 NULL

    while (playAgain) {
        
        clearScreen();
        printf(COLOR_CYAN "=======================================\n");
        printf("         踩地雷 主選單\n");
        printf("=======================================\n" COLOR_RESET);
        printf("1. 開始新遊戲\n");
        printf("2. 載入遊戲\n");
        printf("3. 查看排行榜\n");
        printf("4. 退出遊戲\n");
        int menuChoice = getValidIntegerInput("請輸入你的選擇 (1-4): ", 1, 4);

        if (menuChoice == 1) { // 開始新遊戲
            selectDifficulty(&rows, &cols, &mineCount); // 讓玩家選擇難度
            // 確保遊戲盤面在每次新遊戲開始時都被釋放和重新初始化
            if (board != NULL) {
                freeBoard(board, gameRows); // 使用之前的 gameRows 來釋放
            }
            board = initializeBoard(rows, cols);
            startTime = 0; // 新遊戲重置計時器
            currentLives = PLAYER_LIVES; // 重置生命值
            gameRows = rows; // 更新全局行數
            gameCols = cols; // 更新全局列數
            totalMines = mineCount; // 更新全局地雷數

            int playerExited = gameLoop(board, rows, cols, mineCount);
            if (playerExited) { // 如果玩家在遊戲中途退出或存檔，則不詢問是否再玩
                playAgain = 1; // 回到主選單
                continue;
            }

        } else if (menuChoice == 2) { // 載入遊戲
            if (board != NULL) {
                freeBoard(board, gameRows); // 釋放之前的盤面
            }
            board = loadGame(&rows, &cols, &mineCount); // 載入遊戲
            if (board != NULL) {
                // 注意：loadGame 內部會更新 totalMines, currentLives, startTime, gameRows, gameCols
                int playerExited = gameLoop(board, rows, cols, mineCount);
                if (playerExited) {
                    playAgain = 1; // 回到主選單
                    continue;
                }
            } else {
                printf(COLOR_RED "無法載入遊戲，請確認存檔檔案是否存在且有效。\n" COLOR_RESET);
                printf("按任意鍵返回主選單...");
                getchar(); // 吸收換行符
                getchar(); // 等待用戶輸入
                playAgain = 1; // 返回主選單
                continue;
            }

        } else if (menuChoice == 3) { // 查看排行榜
            displayRanking();
            playAgain = 1; // 返回主選單
            continue;
        } else if (menuChoice == 4) { // 退出遊戲
            playAgain = 0;
            printf(COLOR_CYAN "感謝遊玩踩地雷！再見！\n" COLOR_RESET);
            break; // 退出主迴圈
        }

        // 只有當遊戲勝利或生命值歸零時，才詢問是否再玩一次
        if (menuChoice == 1 || menuChoice == 2) { // 只有這兩個選項會進入 gameLoop
            playAgainPrompt(&playAgain);
        }
    }

    // 在程式結束前釋放最後一次分配的盤面記憶體
    if (board != NULL) {
        freeBoard(board, gameRows);
    }

    return 0;
}