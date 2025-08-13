#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h> 
#include <string.h> 
#include <windows.h>


// --- �`�Ʃw�q ---
#define MAX_ROWS 30
#define MAX_COLS 30
#define DEFAULT_ROWS 9
#define DEFAULT_COLS 9
#define DEFAULT_MINES 10
#define PLAYER_LIVES 3 // �w�]�ͩR��
#define RADAR_COUNT 2 // �p�F��ƶq
#define RADAR_RANGE 2 // �p�F�����b�| (2 ��ܥH���߬��b�A���k�W�U�U���� 2 ��A�`�@ 5x5)

#define SAVE_FILE_NAME "minesweeper_save.dat" // �C���s���ɦW
#define RANKING_FILE_NAME "minesweeper_ranking.txt" // �Ʀ�]�ɦW
#define MAX_RANKING_ENTRIES 10 // �Ʀ�]�̤j���ؼ�
#define MAX_NAME_LENGTH 20 // ���a�m�W�̤j����

// ANSI Escape Codes for colors and text styles (�ȭ��䴩 ANSI ���׺ݾ�)
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define BG_GRAY "\033[47m" // �զ�I���A�Ω�½�}����l
#define BG_DEFAULT "\033[49m"
#define COLOR_BLACK "\033[30m" // �¦�r��A�Ω� BG_GRAY �W�� 8

// �a�p��l�����c
typedef struct {
    int isMine;         // �O�_���a�p
    int isRevealed;     // �O�_�w½�}
    int isFlagged;      // �O�_�w�аO�X�m
    int adjacentMines;  // �P��a�p��
    int isRadar;        // �O�_���p�F��
} Cell;

// �C���s�ɵ��c��
typedef struct {
    int rows;
    int cols;
    int totalMines;
    int currentLives;
    time_t savedTimeOffset; // �C���w�i�檺�ɶ� (�q�}�l��s�ɪ��ɶ��t)
    Cell cells[MAX_ROWS][MAX_COLS]; // �����O�J�L���ƾ�
} GameSave;

// --- �Ʀ�]���ص��c�� ---
typedef struct {
    char name[MAX_NAME_LENGTH + 1]; // ���a�m�W
    long timeTaken;                  // �C���ɶ� (��)
    int rows;                        // �L�����
    int cols;                        // �L���C��
    int mines;                       // �a�p��
} RankingEntry;

// --- �����ܼ� ---
time_t startTime;
time_t endTime;
int currentLives; // ��e�ͩR��
int totalMines;   // �O����e�C�����a�p�`��
int gameRows, gameCols; // �O����e�C������ƩM�C�� (�Ω�s�ɩM�Ʀ�])

// --- �p��X�m�ƶq ---
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

// --- ��ƭ쫬�ŧi ---
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

// --- �s��/Ū�ɨ�ƭ쫬 ---
int saveGame(Cell** board, int rows, int cols);
Cell** loadGame(int *rows, int *cols, int *mineCount);

// --- �Ʀ�]��ƭ쫬 ---
void displayRanking();
void addRankingEntry(const char* name, long timeTaken, int rows, int cols, int mines);
void sortRanking(RankingEntry ranking[], int count);
void saveRanking(RankingEntry ranking[], int count);
int loadRanking(RankingEntry ranking[]);


// �C�����ʻP��ı�ĪG������� 

// �M���׺ݾ��ù�
void clearScreen() {
    system("cls");
}

// �w��T��
void printWelcomeMessage() {
    clearScreen();
    printf(COLOR_CYAN "=======================================\n");
    printf("         �w��Ө��a�p�I\n");
    printf("=======================================\n" COLOR_RESET);
    printf("�ؼСG½�}�Ҧ��D�a�p����l�C\n");
    printf("�ާ@�G\n");
    printf("  'R' �� 'r'�G½�}��l\n");
    printf("  'F' �� 'f'�G�аO/�����аO�X�m\n");
    printf("  'C' �� 'c'�G½�}�P��]�p�G�X�m�Ƥǰt�^\n");
    printf("  'S' �� 's'�G�s��\n"); // �s�W�s�ɴ���
    printf("  'Q' �� 'q'�G���}�C��\n"); // �s�W���}����
    printf("---------------------------------------\n");
}

// ��ܹC���L�����A
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
    // ��ܥͩR�ȡB�X�m�ƻP�g�L�ɶ�
    printf("\n�ͩR��: %d\n", currentLives);
    printf("�X�m��: %d / %d\n", countFlags(board, rows, cols), totalMines);
    if (startTime != 0) {
        printf("�g�L�ɶ�: %ld ��\n", (long)(time(NULL) - startTime));
    }
}

// ��ܩҦ��a�p (�C�������ɥ�)
void revealAllMines(Cell** board, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (board[i][j].isMine) {
                board[i][j].isRevealed = 1;
            }
        }
    }
    displayBoard(board, rows, cols, 1); // �ǤJ 1 �� displayBoard ��ܩҦ��a�p
}


// ��l�ƹC���L��
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
            board[i][j].isRadar = 0; // ��l�ƹp�F�欰�_
        }
    }
    return board;
}

// �H�����m�a�p (�s�W firstClickRow, firstClickCol �ѼơA�Ω�w���}��)
void placeMines(Cell** board, int rows, int cols, int mineCount, int firstClickRow, int firstClickCol) {
    srand(time(NULL));
    int placedMines = 0;
    while (placedMines < mineCount) {
        int row = rand() % rows;
        int col = rand() % cols;

        // �T�O�a�p���|��m�b�Ĥ@���I������l�Ψ�P�� (3x3 �w����)
        if (row >= firstClickRow -1 && row <= firstClickRow + 1 &&
            col >= firstClickCol - 1 && col <= firstClickCol + 1) {
            continue;
        }
        // �T�O���P�p�F�歫�|
        if (board[row][col].isMine == 0 && board[row][col].isRadar == 0) {
            board[row][col].isMine = 1;
            placedMines++;
        }
    }
}

// �H�����m�p�F��
void placeRadarCells(Cell** board, int rows, int cols, int radarCount, int firstClickRow, int firstClickCol) {
    srand(time(NULL));
    int placedRadar = 0;
    while (placedRadar < radarCount) {
        int row = rand() % rows;
        int col = rand() % cols;

        // �T�O�p�F�椣�|��m�b�Ĥ@���I������l�Ψ�P�� (3x3 �w����)
        if (row >= firstClickRow -1 && row <= firstClickRow + 1 &&
            col >= firstClickCol - 1 && col <= firstClickCol + 1) {
            continue;
        }
        // �T�O���P�a�p�Ψ�L�p�F�歫�|
        if (board[row][col].isMine == 0 && board[row][col].isRadar == 0) {
            board[row][col].isRadar = 1;
            placedRadar++;
        }
    }
}

// �p��P��a�p��
void calculateAdjacentMines(Cell** board, int rows, int cols) {
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (board[i][j].isMine == 1) continue;
            if (board[i][j].isRadar == 1) continue; // �p�F�椣�ݭn�p��P��a�p��

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

// ���j½�}��l (�S���a�p�B�P��a�p�Ƭ� 0 ��)
void revealCellRecursive(Cell** board, int rows, int cols, int row, int col) {
    if (row < 0 || row >= rows || col < 0 || col >= cols || board[row][col].isRevealed || board[row][col].isFlagged) {
        return;
    }

    board[row][col].isRevealed = 1;

    // �u�����e��l�P��S���a�p�ɡA�~�~�򻼰j½�}�P���l
    if (board[row][col].adjacentMines == 0 && board[row][col].isRadar == 0) {
        int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
        int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int i = 0; i < 8; i++) {
            revealCellRecursive(board, rows, cols, row + dx[i], col + dy[i]);
        }
    }
}

// �Ұʹp�F��\��
void activateRadar(Cell** board, int rows, int cols, int row, int col) {
    board[row][col].isRevealed = 1;
    printf(COLOR_MAGENTA "�p�F�ҰʡI���y�P��a�p�üаO�I\n" COLOR_RESET);
    // ���y�H (row, col) �����ߪ� 5x5 �ϰ� (�b�|�� RADAR_RANGE)
    for (int i = row - RADAR_RANGE; i <= row + RADAR_RANGE; i++) {
        for (int j = col - RADAR_RANGE; j <= col + RADAR_RANGE; j++) {
            if (i >= 0 && i < rows && j >= 0 && j < cols) {
                // �p�G�O�a�p�B�|���Q½�}�μаO�A�h�۰ʼаO�X�m
                if (board[i][j].isMine && !board[i][j].isRevealed && !board[i][j].isFlagged) {
                    board[i][j].isFlagged = 1; // �۰ʼаO�X�m
                    printf(COLOR_GREEN "�p�F�b (%d, %d) �o�{�üаO�a�p�I\n" COLOR_RESET, i, j);
                }
            }
        }
    }
}

// ½�}��l
void revealCell(Cell** board, int rows, int cols, int row, int col) {
    if (row < 0 || row >= rows || col < 0 || col >= cols || board[row][col].isRevealed || board[row][col].isFlagged) {
        return; // �L�ľާ@
    }

    // �ˬd�O�_�O�Ĥ@���I���A�p�G�O�A�h�}�l�p�ɨçG�p�M�p�F��
    // �`�N�G�p�G�C���O�q�s�ɸ��J���AstartTime�i�ण��0�A���a�p�M�p�F��w�g�G�m
    if (startTime == 0) {
        startTime = time(NULL);
        placeMines(board, rows, cols, totalMines, row, col); // �Ĥ@���I����A�G�p
        placeRadarCells(board, rows, cols, RADAR_COUNT, row, col); // �Ĥ@���I����G�m�p�F��
        calculateAdjacentMines(board, rows, cols); // ���s�p��a�p��
    }

    board[row][col].isRevealed = 1;

    if (board[row][col].isMine) {
        currentLives--; // �ͩR�ȴ�@
        printf(COLOR_RED "�A���a�p�F�I�ͩR�� -1�C�Ѿl�ͩR��: %d\n" COLOR_RESET, currentLives);
        Sleep(2000);
        // �p�G�ͩR���k�s�A�h�C������
        if (currentLives <= 0) {
            printf(COLOR_RED "�ͩR���k�s�I�C�������I\n" COLOR_RESET);
            return; // ��^�A�b gameLoop ���B�z�C������
        }
        // �p�G�٦��ͩR�A�u�O����A��l����ܬ��a�p�A���C���~��
        return;
    }

    if (board[row][col].isRadar) { // �p�G½�}���O�p�F��
        activateRadar(board, rows, cols, row, col);
        return; // �p�F��Q½�}�ᤣ�~�򻼰j½�}��L��l
    }

    // �p�G�P��a�p�Ƭ� 0�A�h���j½�}�P���l
    if (board[row][col].adjacentMines == 0) {
        revealCellRecursive(board, rows, cols, row, col);
    }
}

// �аO�X�m
void flagCell(Cell** board, int rows, int cols, int row, int col) {
    if (row < 0 || row >= rows || col < 0 || col >= cols || board[row][col].isRevealed) {
        return;
    }
    board[row][col].isFlagged = !board[row][col].isFlagged;
}

// �s�W�\��G�p�G�Ʀr��P�򪺺X�m�ƶq�P��Ʀr�ǰt�A�h½�}�P��½�}�B���аO����l
// ��^�ȡG0 ��ܨS��Ĳ�o��Ĳ�o���ѡ]���p�^�A1 ��ܦ��\½�}�P���l
int revealAroundIfFlagsMatch(Cell** board, int rows, int cols, int row, int col) {
    // 1. �ˬd�I������l�O�_�w½�}�B���O�a�p�ιp�F�� (�����O�Ʀr��)
    if (!board[row][col].isRevealed || board[row][col].isMine || board[row][col].isRadar) {
        printf(COLOR_YELLOW "�o�Ӯ�l�L�k�۰�½�}�P��]�����O�w½�}���Ʀr��^�C\n" COLOR_RESET);
        Sleep(1500);
        return 0;
    }

    int adjacentFlags = 0;
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    // 2. �p��P��w�аO���X�m�ƶq
    for (int k = 0; k < 8; k++) {
        int newRow = row + dx[k];
        int newCol = col + dy[k];

        if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
            if (board[newRow][newCol].isFlagged) {
                adjacentFlags++;
            }
        }
    }

    // 3. �ˬd�P��X�m�ƶq�O�_�P��e��l���Ʀr�ǰt
    if (adjacentFlags == board[row][col].adjacentMines) {
        printf(COLOR_GREEN "�P��X�m�ƶq�ǰt�I�۰�½�}�P���l�C\n" COLOR_RESET);
        Sleep(1000);
        // 4. ½�}�P��Ҧ���½�}�B���аO����l
        for (int k = 0; k < 8; k++) {
            int newRow = row + dx[k];
            int newCol = col + dy[k];

            if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
                // �u����½�}�B���аO����l�~�|�Q����½�}
                if (!board[newRow][newCol].isRevealed && !board[newRow][newCol].isFlagged) { // �ץ��� newRow, newCol
                     // �o�̪����I�s revealCell �ӳB�z�i����a�p�γs��½�}�����p
                    revealCell(board, rows, cols, newRow, newCol); 
                    // �p�G���a�p�ɭP�ͩR���k�s�A�h��^ 0
                    if (currentLives <= 0) {
                        return 0; 
                    }
                }
            }
        }
        return 1; // ���\Ĳ�o
    } else {
        printf(COLOR_YELLOW "�P��X�m�ƶq���ǰt�A�L�k�۰�½�}�P���l�C\n" COLOR_RESET);
        Sleep(1500);
        return 0; // ��Ĳ�o
    }
}


// �ˬd�ӭt����
int checkWin(Cell** board, int rows, int cols) {
    int revealedNonMines = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // �Ҧ��D�a�p�B�D�p�F�檺��l���w�Q½�}
            if (board[i][j].isRevealed && !board[i][j].isMine && !board[i][j].isRadar) {
                revealedNonMines++;
            }
        }
    }
    // �ӧQ����G�Ҧ��D�a�p�B�D�p�F�檺��l���Q½�}
    return (revealedNonMines == (rows * cols - totalMines - RADAR_COUNT)); 
}

// ����O����
void freeBoard(Cell** board, int rows) {
    if (board == NULL) return;
    for (int i = 0; i < rows; i++) {
        if (board[i] != NULL) {
            free(board[i]);
        }
    }
    free(board);
}

// ������ľ�ƿ�J
int getValidIntegerInput(const char* prompt, int min, int max) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1) {
            // �M�ſ�J�w�İ�
            while (getchar() != '\n');
            if (value >= min && value <= max) {
                return value;
            } else {
                printf(COLOR_RED "��J�W�X�d��A�п�J %d �� %d �������Ʀr�C\n" COLOR_RESET, min, max);
            }
        } else {
            printf(COLOR_RED "�L�Ŀ�J�A�п�J�@�Ӿ�ơC\n" COLOR_RESET);
            // �M�ſ�J�w�İ�
            while (getchar() != '\n');
        }
    }
}

// �C�����׿��
void selectDifficulty(int *rows, int *cols, int *mineCount) {
    int choice;
    printf("\n�������:\n");
    printf("1. ��� (9x9 �L��, 10 �Ӧa�p)\n");
    printf("2. ���� (16x16 �L��, 40 �Ӧa�p)\n");
    printf("3. ���� (16x30 �L��, 99 �Ӧa�p)\n");
    printf("4. �ۭq\n");

    choice = getValidIntegerInput("�п�J�z����� (1-4): ", 1, 4);



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
            printf("\n--- �ۭq���� ---\n");
            *rows = getValidIntegerInput("��J�C�� (5-30): ", 5, MAX_ROWS);
            *cols = getValidIntegerInput("��J��� (5-30): ", 5, MAX_COLS);
            // ��ĳ�a�p�ƤW�����L�����n��1/3�A�ýT�O���������Ŷ���m�p�F��
            int maxMines = (*rows * *cols) / 3;
            // �T�O�a�p�ƶq���|�ɭP�L�k�}�� (�ܤ֫O�d 9 �Ӧw���� + �p�F��ƶq)
            int minSafeCells = 9 + RADAR_COUNT;
            if (maxMines > (*rows * *cols) - minSafeCells) {
                maxMines = (*rows * *cols) - minSafeCells;
            }
            if (maxMines < 1) maxMines = 1; // �ܤ֥i�H���@�Ӧa�p

            *mineCount = getValidIntegerInput("��J�a�p�ƶq (1-N): ", 1, maxMines);
            
            // �A���ˬd�A�p�G�a�p�ƶq�Ӱ��A�۰ʽվ�
            if (*mineCount >= (*rows * *cols) - minSafeCells) {
                *mineCount = (*rows * *cols) - minSafeCells;
                printf(COLOR_YELLOW "�a�p�ƶq�L�h�A�w�۰ʽվ㬰 %d �ӡA�T�O�i�H�w���}���C\n" COLOR_RESET, *mineCount);
            }
            break;
    }
    printf(COLOR_GREEN "�C���]�w: %d �C x %d ��, %d �Ӧa�p, %d �ӹp�F��\n" COLOR_RESET, *rows, *cols, *mineCount, RADAR_COUNT);
    totalMines = *mineCount; // ��s�����a�p��
    gameRows = *rows; // ��s�������
    gameCols = *cols; // ��s�����C��
}

// �߰ݪ��a�O�_�A���@��
void playAgainPrompt(int *playAgain) {
    char choice;
    printf("\n�O�_�A���@��? (Y/N): ");
    // �T�O�uŪ���@�Ӧr���A�é����Ѿl��J
    while (scanf(" %c", &choice) != 1) {
        printf(COLOR_RED "�L�Ŀ�J�A�п�J Y �� N: " COLOR_RESET);
        while (getchar() != '\n'); // �M�Žw�İ�
    }
    while (getchar() != '\n'); // �M�Žw�İ�
    
    choice = toupper(choice); // �ഫ���j�g

    if (choice == 'Y') {
        *playAgain = 1;
    } else {
        *playAgain = 0;
    }
}

// �x�s�C���i��
int saveGame(Cell** board, int rows, int cols) {
    FILE *fp;
    GameSave save_data;

    // ��R�s�ɼƾ�
    save_data.rows = rows;
    save_data.cols = cols;
    save_data.totalMines = totalMines; // �ϥΥ����ܼ�
    save_data.currentLives = currentLives; // �ϥΥ����ܼ�
    save_data.savedTimeOffset = time(NULL) - startTime; // �p��w���ɶ�

    // �N�ʺA���t���L���ƾڽƻs���R�A�Ʋդ�
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            save_data.cells[i][j] = board[i][j];
        }
    }

    fp = fopen(SAVE_FILE_NAME, "wb"); // �H�G�i��g�Ҧ����}
    if (fp == NULL) {
        perror(COLOR_RED "�L�k�}�Ҧs���ɮסI\n" COLOR_RESET);
        Sleep(2000);
        return 0;
    }

    if (fwrite(&save_data, sizeof(GameSave), 1, fp) != 1) {
        perror(COLOR_RED "�g�J�s�ɥ��ѡI\n" COLOR_RESET);
        Sleep(2000);
        fclose(fp);
        return 0;
    }

    fclose(fp);
    printf(COLOR_GREEN "�C���w�s�ɨ� '%s'�C\n" COLOR_RESET, SAVE_FILE_NAME);
    Sleep(2000);
    return 1;
}

// ���J�C���i��
// ��^�ȡG���J���\���L�� (Cell**)�A���ѫh�� NULL
Cell** loadGame(int *rows, int *cols, int *mineCount) {
    FILE *fp;
    GameSave load_data;

    fp = fopen(SAVE_FILE_NAME, "rb"); // �H�G�i��Ū�Ҧ����}
    if (fp == NULL) {
        perror(COLOR_RED "�L�k�}�Ҧs���ɮשΦs�ɤ��s�b�C\n" COLOR_RESET);
        Sleep(2000);
        return NULL;
    }

    if (fread(&load_data, sizeof(GameSave), 1, fp) != 1) {
        perror(COLOR_RED "Ū���s�ɥ��ѩΦs�ɮ榡�����T�I\n" COLOR_RESET);
        Sleep(2000);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    // ��s�����C�����A
    *rows = load_data.rows;
    *cols = load_data.cols;
    *mineCount = load_data.totalMines; // ��s�ǤJ���a�p�ƫ��w
    totalMines = load_data.totalMines; // ��s�����a�p��
    currentLives = load_data.currentLives; // ��s�����ͩR��
    // ���s�p�� startTime�A�Ϩ�����J�ɤw�����ɶ�
    startTime = time(NULL) - load_data.savedTimeOffset; 
    gameRows = *rows; // ��s�������
    gameCols = *cols; // ��s�����C��

    // ���s�ʺA���t�L���ýƻs���J���ƾ�
    Cell** board = initializeBoard(*rows, *cols);
    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            board[i][j] = load_data.cells[i][j];
        }
    }

    printf(COLOR_GREEN "�C���w�q '%s' ���J�I\n" COLOR_RESET, SAVE_FILE_NAME);
    Sleep(2000);
    return board;
}

// �����ơA�Ω�qsort�Ƨ� (�ɶ��u���Ʀb�e���A�ɶ��ۦP�h���L���j�p��)
int compareRankingEntries(const void *a, const void *b) {
    RankingEntry *entryA = (RankingEntry *)a;
    RankingEntry *entryB = (RankingEntry *)b;

    // �������ɶ��ɧǱƧ�
    if (entryA->timeTaken < entryB->timeTaken) return -1;
    if (entryA->timeTaken > entryB->timeTaken) return 1;

    // �ɶ��ۦP�A���L���j�p�]�`��l�ơ^���ǱƧǡ]�j�L���ۦP�ɶ������^
    int sizeA = entryA->rows * entryA->cols;
    int sizeB = entryB->rows * entryB->cols;
    if (sizeA > sizeB) return -1;
    if (sizeA < sizeB) return 1;

    // �L���j�p�ۦP�A���a�p�ƭ��ǱƧ�
    if (entryA->mines > entryB->mines) return -1;
    if (entryA->mines < entryB->mines) return 1;

    return 0; // �����ۦP
}

// ���J�Ʀ�]�ƾ�
int loadRanking(RankingEntry ranking[]) {
    FILE *fp;
    int count = 0;
    fp = fopen(RANKING_FILE_NAME, "r"); // �HŪ�Ҧ����}
    if (fp == NULL) {
        return 0; // �ɮפ��s�b�A��^0�ӱ���
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

// �O�s�Ʀ�]�ƾ�
void saveRanking(RankingEntry ranking[], int count) {
    FILE *fp;
    fp = fopen(RANKING_FILE_NAME, "w"); // �H�g�Ҧ����} (�|�M���¤��e)
    if (fp == NULL) {
        perror(COLOR_RED "�L�k�O�s�Ʀ�]�ɮסI\n" COLOR_RESET);
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

// �N�s�����Z�[�J�Ʀ�]
void addRankingEntry(const char* name, long timeTaken, int rows, int cols, int mines) {
    RankingEntry ranking[MAX_RANKING_ENTRIES];
    int count = loadRanking(ranking);

    // �إ߷s������
    RankingEntry newEntry;
    strncpy(newEntry.name, name, MAX_NAME_LENGTH);
    newEntry.name[MAX_NAME_LENGTH] = '\0'; // �T�O�r�굲��
    newEntry.timeTaken = timeTaken;
    newEntry.rows = rows;
    newEntry.cols = cols;
    newEntry.mines = mines;

    // �p�G�Ʀ�]�����A�����[�J
    if (count < MAX_RANKING_ENTRIES) {
        ranking[count] = newEntry;
        count++;
    } else {
        // �p�G�Ʀ�]�w���A�ˬd�s���Z�O�_��̮t�����Z�n
        // (���ƧǤ~�ા�D�̮t���O���@��)
        qsort(ranking, count, sizeof(RankingEntry), compareRankingEntries);
        if (compareRankingEntries(&newEntry, &ranking[MAX_RANKING_ENTRIES - 1]) < 0) {
            ranking[MAX_RANKING_ENTRIES - 1] = newEntry; // �������̮t��
        }
    }

    // ���s�ƧǨëO�s
    qsort(ranking, count, sizeof(RankingEntry), compareRankingEntries);
    saveRanking(ranking, count);
}

// ��ܱƦ�]
void displayRanking() {
    clearScreen();
    printf(COLOR_CYAN "=======================================\n");
    printf("             ��a�p �Ʀ�]\n");
    printf("=======================================\n" COLOR_RESET);

    RankingEntry ranking[MAX_RANKING_ENTRIES];
    int count = loadRanking(ranking);

    if (count == 0) {
        printf("�ثe�S������Ʀ�]�O���C\n");
    } else {
        printf("No. �m�W                 �ɶ�(��) �L��\n");
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
    printf("�����N���^�D���...");
    getchar(); // �l�������
    getchar(); // ���ݥΤ��J
}

// �C���j��
// ��^�ȡG0 �C�����`���� (���Ĺ)�A1 ���a��ܰh�X�Φs��
int gameLoop(Cell** board, int rows, int cols, int mineCount) {
    int gameOver = 0;
    char operation_char; // �ϥ� char �ӱ����ާ@���O
    int row, col;
    int gameExit = 0; // �лx�O�_�O���a��ܰh�X�Φs��

    // �p�G�O�q�s�ɸ��J�AstartTime�w�g�bloadGame���]�m�A�_�h��0
    // �p�G�O�s�C���A�h�b�Ĥ@���I���ɳ]�mstartTime

    displayBoard(board, rows, cols, 0);

    while (!gameOver && !gameExit) {
        printf("\n�п�J�ާ@ (R: ½�}, F: �аO/�����аO, C: �M�ũP��, S: �s��, Q: ���}) �Φ�C�� (�Ҧp: R 0 0): ");
        char inputBuffer[50]; // �w�İϥΩ�Ū������J
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL) {
            printf(COLOR_RED "��J���~�A�Э��s��J�C\n" COLOR_RESET);
            continue;
        }

        // �ѪR��J
        int scanned_items = sscanf(inputBuffer, " %c %d %d", &operation_char, &row, &col);
        operation_char = toupper(operation_char);

        if (scanned_items < 1) { // �ܤ֭n��J�ާ@���O
             printf(COLOR_RED "��J�榡���~�A�Э��s��J�C\n" COLOR_RESET);
             continue;
        }
        
        // �B�z���a��C�����ާ@ (S, Q)
        if (operation_char == 'S') {
            if (startTime == 0) {
                printf(COLOR_YELLOW "�C���|���}�l�A�L�k�s�ɡC\n" COLOR_RESET);
                continue;
            }
            if (saveGame(board, rows, cols)) {
                gameExit = 1; // �s�ɫ�h�X�C���j��
            }
            continue;
        }
        if (operation_char == 'Q') {
            printf(COLOR_YELLOW "�A�T�w�n���}�C���ܡH(Y/N): " COLOR_RESET);
            char confirm_char;
            if (scanf(" %c", &confirm_char) == 1) {
                while(getchar() != '\n'); // �M�Žw�İ�
                if (toupper(confirm_char) == 'Y') {
                    gameExit = 1; // ���a�T�{���}
                }
            } else {
                while(getchar() != '\n'); // �M�Žw�İ�
            }
            if (gameExit) continue; // �p�G�T�w���}�A���L����B�z
            // �p�G���T�w���}�A���s��ܽL��
            displayBoard(board, rows, cols, 0);
            continue;
        }

        // �B�z�ݭn��C�����ާ@ (R, F, C)
        if (scanned_items != 3) {
            printf(COLOR_RED "��J�榡���~�A�Э��s��J (�Ҧp: R 0 0)�C\n" COLOR_RESET);
            continue;
        }

        if (row < 0 || row >= rows || col < 0 || col >= cols) {
            printf(COLOR_RED "��J����C���W�X�d�� (%d-%d, %d-%d)�C\n" COLOR_RESET, 0, rows - 1, 0, cols - 1);
            continue;
        }

        if (operation_char == 'R') {
            // �b�Ĥ@���I���ɤ~�G�m�a�p�M�p�F��A�íp��P��a�p��
            if (startTime == 0) {
                startTime = time(NULL);
                placeMines(board, rows, cols, totalMines, row, col);
                placeRadarCells(board, rows, cols, RADAR_COUNT, row, col);
                calculateAdjacentMines(board, rows, cols);
            }

            // �ˬd�O�_�w½�}�Τw�аO
            if (board[row][col].isRevealed || board[row][col].isFlagged) {
                printf(COLOR_YELLOW "�Ӯ�l�w�g½�}�Τw�аO�X�m�A�п�ܨ�L��l�C\n" COLOR_RESET);
                Sleep(2000);
            } else {
                revealCell(board, rows, cols, row, col);
                // �ˬd�O�_�]���a�p�ӥͩR���k�s
                if (board[row][col].isMine && currentLives <= 0) { // �T�O�T��O���a�p�B�ͩR���k�s
                    gameOver = 1;
                    printf(COLOR_RED "�A�w���h�Ҧ��ͩR�I�C�������I\n" COLOR_RESET);
                    endTime = time(NULL);
                    printf("�`�Ӯ�: %ld ��\n", (long)(endTime - startTime));
                    Sleep(3000);
                    revealAllMines(board, rows, cols); // ��ܩҦ��a�p
                }
            }
        } else if (operation_char == 'F') {
            flagCell(board, rows, cols, row, col);
        } else if (operation_char == 'C') { // �s�W 'C' �ާ@
            // �p�G�O�Ĥ@���I���A���ܵL�k�ϥΦ��\��
            if (startTime == 0) {
                printf(COLOR_YELLOW "�C���|���}�l�A�L�k�ϥΡC�Х�½�}�@�Ӯ�l�C\n" COLOR_RESET);
                continue;
            }
            int result = revealAroundIfFlagsMatch(board, rows, cols, row, col);
            // �p�G�]���۰�½�}�ӽ��p�åͩR���k�s
            if (result == 0 && currentLives <= 0) {
                 gameOver = 1;
                 printf(COLOR_RED "�A�w���h�Ҧ��ͩR�I�C�������I\n" COLOR_RESET);
                 endTime = time(NULL);
                 printf("�`�Ӯ�: %ld ��\n", (long)(endTime - startTime));
                 Sleep(3000);
                 revealAllMines(board, rows, cols); // ��ܩҦ��a�p
            }
        }
        else {
            printf(COLOR_RED "�L�Ī��ާ@�A�п�J 'R', 'F', 'C', 'S' �� 'Q'�C\n" COLOR_RESET);
            continue;
        }

        // �C���ާ@����ܽL�� (�Ȧb�C���������B���h�X��)
        if (!gameOver && !gameExit) {
            displayBoard(board, rows, cols, 0);
        }

        // �ˬd�ӧQ���� (�Ȧb�C���|���������ˬd)
        if (!gameOver && !gameExit && checkWin(board, rows, cols)) {
            printf(COLOR_GREEN "���ߧA�I�AĹ�F�I\n" COLOR_RESET);
            endTime = time(NULL);
            long time_taken = (long)(endTime - startTime);
            printf("�`�Ӯ�: %ld ��\n", time_taken);

            // �߰ݪ��a�m�W�å[�J�Ʀ�]
            char playerName[MAX_NAME_LENGTH + 1];
            printf("�п�J�A���m�W (�̦h %d �Ӧr��): ", MAX_NAME_LENGTH);
            // �ϥ� fgets Ū�����A�קK�Ů���D�A�òM�z�����
            if (fgets(playerName, sizeof(playerName), stdin) != NULL) {
                playerName[strcspn(playerName, "\n")] = 0; // ���������
            } else {
                strcpy(playerName, "Unnamed"); // Ū�����ѵ��ӹw�]�W
            }
            addRankingEntry(playerName, time_taken, rows, cols, mineCount);

            gameOver = 1; // �C���ӧQ����
        }
    }
    return gameExit; // ��^�O�_�O���a�D�ʰh�X
}

int main() {
    int rows, cols, mineCount;
    int playAgain = 1;
    Cell** board = NULL; // ��l�Ƭ� NULL

    while (playAgain) {
        
        clearScreen();
        printf(COLOR_CYAN "=======================================\n");
        printf("         ��a�p �D���\n");
        printf("=======================================\n" COLOR_RESET);
        printf("1. �}�l�s�C��\n");
        printf("2. ���J�C��\n");
        printf("3. �d�ݱƦ�]\n");
        printf("4. �h�X�C��\n");
        int menuChoice = getValidIntegerInput("�п�J�A����� (1-4): ", 1, 4);

        if (menuChoice == 1) { // �}�l�s�C��
            selectDifficulty(&rows, &cols, &mineCount); // �����a�������
            // �T�O�C���L���b�C���s�C���}�l�ɳ��Q����M���s��l��
            if (board != NULL) {
                freeBoard(board, gameRows); // �ϥΤ��e�� gameRows ������
            }
            board = initializeBoard(rows, cols);
            startTime = 0; // �s�C�����m�p�ɾ�
            currentLives = PLAYER_LIVES; // ���m�ͩR��
            gameRows = rows; // ��s�������
            gameCols = cols; // ��s�����C��
            totalMines = mineCount; // ��s�����a�p��

            int playerExited = gameLoop(board, rows, cols, mineCount);
            if (playerExited) { // �p�G���a�b�C�����~�h�X�Φs�ɡA�h���߰ݬO�_�A��
                playAgain = 1; // �^��D���
                continue;
            }

        } else if (menuChoice == 2) { // ���J�C��
            if (board != NULL) {
                freeBoard(board, gameRows); // ���񤧫e���L��
            }
            board = loadGame(&rows, &cols, &mineCount); // ���J�C��
            if (board != NULL) {
                // �`�N�GloadGame �����|��s totalMines, currentLives, startTime, gameRows, gameCols
                int playerExited = gameLoop(board, rows, cols, mineCount);
                if (playerExited) {
                    playAgain = 1; // �^��D���
                    continue;
                }
            } else {
                printf(COLOR_RED "�L�k���J�C���A�нT�{�s���ɮ׬O�_�s�b�B���ġC\n" COLOR_RESET);
                printf("�����N���^�D���...");
                getchar(); // �l�������
                getchar(); // ���ݥΤ��J
                playAgain = 1; // ��^�D���
                continue;
            }

        } else if (menuChoice == 3) { // �d�ݱƦ�]
            displayRanking();
            playAgain = 1; // ��^�D���
            continue;
        } else if (menuChoice == 4) { // �h�X�C��
            playAgain = 0;
            printf(COLOR_CYAN "�P�¹C����a�p�I�A���I\n" COLOR_RESET);
            break; // �h�X�D�j��
        }

        // �u����C���ӧQ�ΥͩR���k�s�ɡA�~�߰ݬO�_�A���@��
        if (menuChoice == 1 || menuChoice == 2) { // �u���o��ӿﶵ�|�i�J gameLoop
            playAgainPrompt(&playAgain);
        }
    }

    // �b�{�������e����̫�@�����t���L���O����
    if (board != NULL) {
        freeBoard(board, gameRows);
    }

    return 0;
}