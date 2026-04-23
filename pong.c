#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#define MODE_BOT 1
#define MODE_TWO_PLAYER 2
#define MODE_SCOREBOARD 3

#define DIFF_EASY 1
#define DIFF_BALANCED 2
#define DIFF_AGGRESSIVE 3

#define WIN_SCORE 5
#define NAME_SIZE 50

/* ---------------------------------------------------------- */
/* String'in sonundaki \n karakterini temizler                */
/* ---------------------------------------------------------- */
void trim_newline(char *str) {
    str[strcspn(str, "\n")] = '\0';
}

/* ---------------------------------------------------------- */
/* Dosyaya yeni maç sonucunu ekler                            */
/* ---------------------------------------------------------- */
void append_scoreboard(const char *leftName, const char *rightName, int leftScore, int rightScore, const char *winnerName) {
    FILE *file = fopen("scoreboard.txt", "a");

    if (file == NULL) {
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(
        file,
        "%02d-%02d-%04d %02d:%02d | %s %d - %d %s | Kazanan: %s\n",
        t->tm_mday,
        t->tm_mon + 1,
        t->tm_year + 1900,
        t->tm_hour,
        t->tm_min,
        leftName,
        leftScore,
        rightScore,
        rightName,
        winnerName
    );

    fclose(file);
}

/* ---------------------------------------------------------- */
/* Topu merkeze alır ve servis yönünü ayarlar                 */
/* serveDirection: -1 sola, 1 sağa                            */
/* ---------------------------------------------------------- */
void reset_ball(int *ballX, int *ballY, int *dirX, int *dirY, int serveDirection) {
    *ballX = COLS / 2;
    *ballY = LINES / 2;
    *dirX = serveDirection;
    *dirY = (rand() % 2 == 0) ? 1 : -1;
}

/* ---------------------------------------------------------- */
/* Oyun başlamadan önce geri sayım                            */
/* ---------------------------------------------------------- */
void countdown_screen(void) {
    for (int i = 3; i >= 1; i--) {
        clear();
        box(stdscr, 0, 0);
        mvprintw(LINES / 2 - 1, COLS / 2 - 10, "Oyun basliyor...");
        mvprintw(LINES / 2, COLS / 2, "%d", i);
        refresh();
        sleep(1);
    }

    clear();
    box(stdscr, 0, 0);
    mvprintw(LINES / 2, COLS / 2 - 3, "Basla!");
    refresh();
    usleep(500000);
}

/* ---------------------------------------------------------- */
/* Ortak ok-tusu menüsü                                       */
/* Dönüş: seçilen index, q ise -1                             */
/* ---------------------------------------------------------- */
int arrow_menu(const char *title, const char *items[], int count) {
    int selected = 0;
    int ch;

    nodelay(stdscr, FALSE);

    while (1) {
        clear();
        box(stdscr, 0, 0);

        mvprintw(LINES / 2 - (count + 3), COLS / 2 - (int)strlen(title) / 2, "%s", title);

        for (int i = 0; i < count; i++) {
            if (i == selected) {
                attron(A_REVERSE);
                mvprintw(LINES / 2 - 1 + i, COLS / 2 - 18, "%s", items[i]);
                attroff(A_REVERSE);
            } else {
                mvprintw(LINES / 2 - 1 + i, COLS / 2 - 18, "%s", items[i]);
            }
        }

        mvprintw(LINES / 2 + count + 1, COLS / 2 - 20, "Yukari/Asagi ile sec, Enter ile onayla");
        mvprintw(LINES / 2 + count + 2, COLS / 2 - 8, "Q ile cikis");

        refresh();

        ch = getch();

        if (ch == 'q' || ch == 'Q') {
            return -1;
        } else if (ch == KEY_UP) {
            selected--;
            if (selected < 0) {
                selected = count - 1;
            }
        } else if (ch == KEY_DOWN) {
            selected++;
            if (selected >= count) {
                selected = 0;
            }
        } else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13) {
            return selected;
        }
    }
}

/* ---------------------------------------------------------- */
/* Ana menü                                                   */
/* ---------------------------------------------------------- */
int main_menu(void) {
    const char *items[] = {
        "Bota karsi oyna",
        "2 kisilik oyna",
        "Skorboard",
        "Cikis"
    };

    int selected = arrow_menu("PONG MENU", items, 4);

    if (selected == -1 || selected == 3) {
        return -1;
    } else if (selected == 0) {
        return MODE_BOT;
    } else if (selected == 1) {
        return MODE_TWO_PLAYER;
    } else {
        return MODE_SCOREBOARD;
    }
}

/* ---------------------------------------------------------- */
/* Zorluk menüsü                                              */
/* ---------------------------------------------------------- */
int difficulty_menu(void) {
    const char *items[] = {
        "Kolay",
        "Orta",
        "Zor"
    };

    int selected = arrow_menu("ZORLUK SECIMI", items, 3);

    if (selected == -1) {
        return -1;
    } else if (selected == 0) {
        return DIFF_EASY;
    } else if (selected == 1) {
        return DIFF_BALANCED;
    } else {
        return DIFF_AGGRESSIVE;
    }
}

/* ---------------------------------------------------------- */
/* Pause menüsü                                               */
/* 0 devam, 1 ana menü, 2 çıkış                               */
/* ---------------------------------------------------------- */
int pause_menu(void) {
    const char *items[] = {
        "Devam et",
        "Ana menuye don",
        "Oyundan cik"
    };

    return arrow_menu("PAUSE", items, 3);
}

/* ---------------------------------------------------------- */
/* Zorluğa göre başlangıç hızı                                */
/* ---------------------------------------------------------- */
int get_base_delay(int difficulty) {
    if (difficulty == DIFF_EASY) {
        return 60000;
    } else if (difficulty == DIFF_BALANCED) {
        return 50000;
    } else {
        return 42000;
    }
}

/* ---------------------------------------------------------- */
/* Zorluğa göre minimum delay                                 */
/* ---------------------------------------------------------- */
int get_min_delay(int difficulty) {
    if (difficulty == DIFF_EASY) {
        return 26000;
    } else if (difficulty == DIFF_BALANCED) {
        return 18000;
    } else {
        return 12000;
    }
}

/* ---------------------------------------------------------- */
/* Hızlanma eğrisi                                            */
/* ---------------------------------------------------------- */
int calculate_delay(int difficulty, int hitCount) {
    int baseDelay = get_base_delay(difficulty);
    int minDelay = get_min_delay(difficulty);
    int delay;

    if (difficulty == DIFF_EASY) {
        delay = baseDelay - (hitCount * 900);
    } else if (difficulty == DIFF_BALANCED) {
        delay = baseDelay - (hitCount * 1400);
    } else {
        delay = baseDelay - (hitCount * 1200) - (hitCount * hitCount * 35);
    }

    if (delay < minDelay) {
        delay = minDelay;
    }

    return delay;
}

/* ---------------------------------------------------------- */
/* Ekranda kullanıcıdan isim alma                             */
/* ---------------------------------------------------------- */
void input_name_screen(const char *prompt, char *buffer, int size) {
    int x = COLS / 2 - 20;
    int y = LINES / 2;

    nodelay(stdscr, FALSE);
    echo();
    curs_set(1);

    clear();
    box(stdscr, 0, 0);

    mvprintw(LINES / 2 - 2, COLS / 2 - (int)strlen(prompt) / 2, "%s", prompt);
    mvprintw(y, x, "> ");
    refresh();

    move(y, x + 2);
    getnstr(buffer, size - 1);

    trim_newline(buffer);

    if (strlen(buffer) == 0) {
        snprintf(buffer, size, "Oyuncu");
    }

    noecho();
    curs_set(0);
}

/* ---------------------------------------------------------- */
/* Skorboard ekranı                                           */
/* ---------------------------------------------------------- */
void scoreboard_screen(void) {
    FILE *file = fopen("scoreboard.txt", "r");
    char line[256];
    int row = 3;

    nodelay(stdscr, FALSE);

    clear();
    box(stdscr, 0, 0);
    mvprintw(1, COLS / 2 - 5, "SKORBOARD");

    if (file == NULL) {
        mvprintw(3, 3, "Henuz kayitli mac yok.");
    } else {
        while (fgets(line, sizeof(line), file) != NULL && row < LINES - 2) {
            trim_newline(line);
            mvprintw(row, 2, "%s", line);
            row++;
        }
        fclose(file);
    }

    mvprintw(LINES - 2, 2, "Devam etmek icin bir tusa bas...");
    refresh();
    getch();
}

/* ---------------------------------------------------------- */
/* Kazanan duyuru ekranı                                      */
/* Birkaç saniye gösterir                                      */
/* ---------------------------------------------------------- */
void winner_announce_screen(const char *winnerName) {
    clear();
    box(stdscr, 0, 0);

    mvprintw(LINES / 2 - 1, COLS / 2 - 6, "OYUN BITTI");
    mvprintw(
        LINES / 2 + 1,
        COLS / 2 - ((int)strlen(winnerName) + 9) / 2,
        "%s kazandi!",
        winnerName
    );

    refresh();
    sleep(3);
}

/* ---------------------------------------------------------- */
/* Kazanan sonrası seçim ekranı                               */
/* 0 ana menü, 1 çıkış                                        */
/* ---------------------------------------------------------- */
int post_winner_menu(void) {
    const char *items[] = {
        "Ana menuye don",
        "Oyundan cik"
    };

    int selected = arrow_menu("MAC SONU", items, 2);

    if (selected == -1 || selected == 1) {
        return 1;
    }

    return 0;
}

int main(void) {
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    srand(time(NULL));

    while (1) {
        int choice = main_menu();

        if (choice == -1) {
            break;
        }

        if (choice == MODE_SCOREBOARD) {
            scoreboard_screen();
            continue;
        }

        int difficulty = difficulty_menu();
        if (difficulty == -1) {
            break;
        }

        char leftName[NAME_SIZE];
        char rightName[NAME_SIZE];

        if (choice == MODE_TWO_PLAYER) {
            input_name_screen("W/S ile oynayan oyuncu adini gir:", leftName, NAME_SIZE);
            input_name_screen("Yon tuslariyla oynayan oyuncu adini gir:", rightName, NAME_SIZE);
        } else {
            snprintf(leftName, NAME_SIZE, "Oyuncu");
            snprintf(rightName, NAME_SIZE, "Bot");
        }

        int ballX, ballY;
        int dirX, dirY;

        int leftPaddleX = 2;
        int leftPaddleY = 8;

        int rightPaddleX = COLS - 3;
        int rightPaddleY = 8;

        int paddleHeight = 4;

        int leftScore = 0;
        int rightScore = 0;

        int delay = get_base_delay(difficulty);
        int hitCount = 0;
        int ch;
        int shouldExitProgram = 0;

        reset_ball(&ballX, &ballY, &dirX, &dirY, (rand() % 2 == 0) ? 1 : -1);

        countdown_screen();
        nodelay(stdscr, TRUE);

        while (1) {
            ch = getch();

            if (ch == 'p' || ch == 'P') {
                int pauseChoice = pause_menu();
                nodelay(stdscr, TRUE);

                if (pauseChoice == 1) {
                    break;
                } else if (pauseChoice == 2 || pauseChoice == -1) {
                    shouldExitProgram = 1;
                    break;
                }
            }

            if (ch == 'q' || ch == 'Q') {
                shouldExitProgram = 1;
                break;
            }

            /* Sol oyuncu hareketi */
            if ((ch == 'w' || ch == 'W') && leftPaddleY > 1) {
                leftPaddleY--;
            } else if ((ch == 's' || ch == 'S') && leftPaddleY + paddleHeight < LINES - 1) {
                leftPaddleY++;
            }

            /* Sağ taraf */
            if (choice == MODE_BOT) {
                int rightCenter = rightPaddleY + paddleHeight / 2;

                if (difficulty == DIFF_EASY) {
                    if (rand() % 3 == 0) {
                        if (ballY < rightCenter && rightPaddleY > 1) {
                            rightPaddleY--;
                        } else if (ballY > rightCenter && rightPaddleY + paddleHeight < LINES - 1) {
                            rightPaddleY++;
                        }
                    }
                } else if (difficulty == DIFF_BALANCED) {
                    if (ballY < rightCenter && rightPaddleY > 1) {
                        rightPaddleY--;
                    } else if (ballY > rightCenter && rightPaddleY + paddleHeight < LINES - 1) {
                        rightPaddleY++;
                    }
                } else {
                    if (ballY < rightCenter && rightPaddleY > 1) {
                        rightPaddleY--;
                    }
                    if (ballY > rightCenter && rightPaddleY + paddleHeight < LINES - 1) {
                        rightPaddleY++;
                    }
                }
            } else {
                if (ch == KEY_UP && rightPaddleY > 1) {
                    rightPaddleY--;
                } else if (ch == KEY_DOWN && rightPaddleY + paddleHeight < LINES - 1) {
                    rightPaddleY++;
                }
            }

            /* Top hareketi */
            ballX += dirX;
            ballY += dirY;

            /* Duvar sekmesi */
            if (ballY <= 1) {
                ballY = 1;
                dirY = 1;
            } else if (ballY >= LINES - 2) {
                ballY = LINES - 2;
                dirY = -1;
            }

            /* Sol paddle çarpışması */
            if (ballX == leftPaddleX + 1 &&
                ballY >= leftPaddleY &&
                ballY < leftPaddleY + paddleHeight) {
                dirX = 1;

                int hitPos = ballY - leftPaddleY;
                if (hitPos == 0) {
                    dirY = -1;
                } else if (hitPos == paddleHeight - 1) {
                    dirY = 1;
                }

                hitCount++;
                delay = calculate_delay(difficulty, hitCount);
            }

            /* Sağ paddle çarpışması */
            if (ballX == rightPaddleX - 1 &&
                ballY >= rightPaddleY &&
                ballY < rightPaddleY + paddleHeight) {
                dirX = -1;

                int hitPos = ballY - rightPaddleY;
                if (hitPos == 0) {
                    dirY = -1;
                } else if (hitPos == paddleHeight - 1) {
                    dirY = 1;
                }

                hitCount++;
                delay = calculate_delay(difficulty, hitCount);
            }

            /* Sol kaçırırsa sağ sayı alır */
            if (ballX <= 1) {
                rightScore++;

                if (rightScore >= WIN_SCORE) {
                    append_scoreboard(leftName, rightName, leftScore, rightScore, rightName);
                    winner_announce_screen(rightName);

                    int after = post_winner_menu();
                    if (after == 1) {
                        shouldExitProgram = 1;
                    }
                    break;
                }

                hitCount = 0;
                delay = get_base_delay(difficulty);
                reset_ball(&ballX, &ballY, &dirX, &dirY, -1);
                usleep(500000);
            }

            /* Sağ kaçırırsa sol sayı alır */
            if (ballX >= COLS - 2) {
                leftScore++;

                if (leftScore >= WIN_SCORE) {
                    append_scoreboard(leftName, rightName, leftScore, rightScore, leftName);
                    winner_announce_screen(leftName);

                    int after = post_winner_menu();
                    if (after == 1) {
                        shouldExitProgram = 1;
                    }
                    break;
                }

                hitCount = 0;
                delay = get_base_delay(difficulty);
                reset_ball(&ballX, &ballY, &dirX, &dirY, 1);
                usleep(500000);
            }

            /* Çizim */
            clear();
            box(stdscr, 0, 0);

            for (int y = 1; y < LINES - 1; y++) {
                if (y % 2 == 0) {
                    mvprintw(y, COLS / 2, "|");
                }
            }

            for (int i = 0; i < paddleHeight; i++) {
                mvprintw(leftPaddleY + i, leftPaddleX, "|");
                mvprintw(rightPaddleY + i, rightPaddleX, "|");
            }

            mvprintw(ballY, ballX, "O");

            mvprintw(1, 3, "%s", leftName);
            mvprintw(1, COLS / 2 - 2, "%d : %d", leftScore, rightScore);
            mvprintw(1, COLS - (int)strlen(rightName) - 4, "%s", rightName);

            if (choice == MODE_BOT) {
                mvprintw(0, COLS / 2 - 30, " W/S hareket | P pause | Mod: Bota karsi | Q cikis ");
            } else {
                mvprintw(0, COLS / 2 - 38, " Sol: W/S | Sag: Yon tuslari | P pause | 2 Kisilik | Q cikis ");
            }

            refresh();
            usleep(delay);
        }

        if (shouldExitProgram) {
            break;
        }
    }

    endwin();
    return 0;
}