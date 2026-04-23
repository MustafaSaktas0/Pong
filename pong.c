#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

/*
    Gelişmiş terminal Pong oyunu (ncurses)

    Özellikler:
    - Ana menüde ok tuşlarıyla seçim
    - Oyun modu seçimi:
        * Bota karşı
        * 2 kişilik
    - Zorluk seçimi:
        * Kolay
        * Dengeli
        * Agresif
    - Oyun başında tek seferlik geri sayım
    - Skor sonrası sadece 0.5 saniye bekleme
    - İlk 5'e ulaşan kazanır
    - Kazanan ekranı
    - Pause menüsü
    - 2 kişilik modda oyuncu isimlerini players.txt dosyasından okuma
    - Skorları scores.txt dosyasına yazma
*/

/* ---------------- SABITLER ---------------- */

#define MODE_BOT 1
#define MODE_TWO_PLAYER 2

#define DIFF_EASY 1
#define DIFF_BALANCED 2
#define DIFF_AGGRESSIVE 3

#define WIN_SCORE 5

/* ---------------- YARDIMCI FONKSIYONLAR ---------------- */

/*
    players.txt dosyasından iki oyuncu adı okur.
    Dosya yoksa varsayılan isimleri kullanır.

    players.txt örnek:
    Ali
    Veli
*/
void load_player_names(char leftName[], char rightName[], size_t size) {
    FILE *file = fopen("players.txt", "r");

    /* Dosya yoksa varsayılan isimler */
    if (file == NULL) {
        snprintf(leftName, size, "Oyuncu 1");
        snprintf(rightName, size, "Oyuncu 2");
        return;
    }

    /* İlk satırı sol oyuncu adı olarak oku */
    if (fgets(leftName, size, file) == NULL) {
        snprintf(leftName, size, "Oyuncu 1");
    }

    /* İkinci satırı sağ oyuncu adı olarak oku */
    if (fgets(rightName, size, file) == NULL) {
        snprintf(rightName, size, "Oyuncu 2");
    }

    /* fgets satır sonunu da aldığı için temizliyoruz */
    leftName[strcspn(leftName, "\n")] = '\0';
    rightName[strcspn(rightName, "\n")] = '\0';

    fclose(file);

    /* Eğer isim boş geldiyse varsayılan ver */
    if (strlen(leftName) == 0) {
        snprintf(leftName, size, "Oyuncu 1");
    }

    if (strlen(rightName) == 0) {
        snprintf(rightName, size, "Oyuncu 2");
    }
}

/*
    Skoru scores.txt dosyasına yazar.
    Küçük bir database gibi düşün.
*/
void save_score_to_file(const char *leftName, const char *rightName, int leftScore, int rightScore) {
    FILE *file = fopen("scores.txt", "w");

    if (file == NULL) {
        return;
    }

    fprintf(file, "%s: %d\n", leftName, leftScore);
    fprintf(file, "%s: %d\n", rightName, rightScore);

    fclose(file);
}

/*
    Oyun başlamadan önce tek seferlik geri sayım.
*/
void countdown_screen(void) {
    for (int i = 3; i >= 1; i--) {
        clear();
        box(stdscr, 0, 0);

        mvprintw(LINES / 2 - 1, COLS / 2 - 10, "Oyun basliyor...");
        mvprintw(LINES / 2,     COLS / 2,      "%d", i);

        refresh();
        sleep(1);
    }

    clear();
    box(stdscr, 0, 0);
    mvprintw(LINES / 2, COLS / 2 - 3, "Basla!");
    refresh();
    usleep(500000);
}

/*
    Ok tuşlarıyla seçim yapılan ortak menü fonksiyonu.

    title: menü başlığı
    items: seçenek metinleri
    count: seçenek sayısı

    Dönüş:
    0,1,2... -> seçilen index
    -1 -> kullanıcı q ile çıkmak istedi
*/
int arrow_menu(const char *title, const char *items[], int count) {
    int selected = 0;
    int ch;

    /* Menüde tuş beklemek istiyoruz */
    nodelay(stdscr, FALSE);

    while (1) {
        clear();
        box(stdscr, 0, 0);

        mvprintw(LINES / 2 - (count + 3), COLS / 2 - (int)strlen(title) / 2, "%s", title);

        for (int i = 0; i < count; i++) {
            if (i == selected) {
                /* Seçili olan satırı highlight yap */
                attron(A_REVERSE);
                mvprintw(LINES / 2 - 1 + i, COLS / 2 - 15, "%s", items[i]);
                attroff(A_REVERSE);
            } else {
                mvprintw(LINES / 2 - 1 + i, COLS / 2 - 15, "%s", items[i]);
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

/*
    Oyun modu seçimi:
    0 -> bota karşı
    1 -> 2 kişilik

    Bunu MODE_BOT / MODE_TWO_PLAYER'e çevireceğiz.
*/
int mode_menu(void) {
    const char *items[] = {
        "Bota karsi oyna",
        "2 kisilik oyna"
    };

    int selected = arrow_menu("PONG MENU", items, 2);

    if (selected == -1) {
        return -1;
    } else if (selected == 0) {
        return MODE_BOT;
    } else {
        return MODE_TWO_PLAYER;
    }
}

/*
    Zorluk seçimi:
    0 -> kolay
    1 -> dengeli
    2 -> agresif
*/
int difficulty_menu(void) {
    const char *items[] = {
        "Kolay",
        "Dengeli",
        "Agresif"
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

/*
    Pause menüsü.
    Dönüş:
    0 -> oyuna devam
    1 -> ana menüye dön
    2 -> çıkış
*/
int pause_menu(void) {
    const char *items[] = {
        "Devam et",
        "Ana menuye don",
        "Oyundan cik"
    };

    return arrow_menu("PAUSE", items, 3);
}

/*
    Kazanan ekranı.
    Dönüş:
    0 -> ana menüye dön
    1 -> oyundan çık
*/
int winner_screen(const char *winnerName) {
    const char *items[] = {
        "Ana menuye don",
        "Oyundan cik"
    };

    while (1) {
        int selected;
        clear();
        box(stdscr, 0, 0);

        mvprintw(LINES / 2 - 3, COLS / 2 - 10, "OYUN BITTI!");
        mvprintw(LINES / 2 - 1, COLS / 2 - ((int)strlen(winnerName) + 12) / 2, "%s kazandi!", winnerName);

        refresh();

        selected = arrow_menu("NE YAPMAK ISTERSIN?", items, 2);

        return selected;
    }
}

/*
    Zorluğa göre başlangıç hızı.
    delay küçükse top daha hızlı gider.
*/
int get_base_delay(int difficulty) {
    if (difficulty == DIFF_EASY) {
        return 60000;
    } else if (difficulty == DIFF_BALANCED) {
        return 50000;
    } else {
        return 42000;
    }
}

/*
    Zorluğa göre en düşük delay değeri.
*/
int get_min_delay(int difficulty) {
    if (difficulty == DIFF_EASY) {
        return 26000;
    } else if (difficulty == DIFF_BALANCED) {
        return 18000;
    } else {
        return 12000;
    }
}

/*
    Zorluğa göre hızlanma eğrisi.
*/
int calculate_delay(int difficulty, int hitCount) {
    int baseDelay = get_base_delay(difficulty);
    int minDelay  = get_min_delay(difficulty);
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

/*
    Topu merkeze alır ve servis yönünü ayarlar.

    serveDirection:
    -1 -> sola
     1 -> sağa
*/
void reset_ball(int *ballX, int *ballY, int *dirX, int *dirY, int serveDirection) {
    *ballX = COLS / 2;
    *ballY = LINES / 2;
    *dirX = serveDirection;
    *dirY = (rand() % 2 == 0) ? 1 : -1;
}

int main(void) {
    /* ncurses başlat */
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    /* random seed */
    srand(time(NULL));

    /* Oyuncu isimleri */
    char leftName[50] = "Oyuncu 1";
    char rightName[50] = "Bot";

    /* Programın ana döngüsü:
       Oyun bitince tekrar ana menüye dönebilelim diye dış döngü kuruyoruz.
    */
    while (1) {
        int gameMode;
        int difficulty;

        /* Top bilgileri */
        int ballX, ballY;
        int dirX, dirY;

        /* Paddle bilgileri */
        int leftPaddleX = 2;
        int leftPaddleY = 8;
        int rightPaddleX;
        int rightPaddleY = 8;
        int paddleHeight = 4;

        /* Skor */
        int leftScore = 0;
        int rightScore = 0;

        /* Hız kontrol */
        int delay;
        int hitCount = 0;

        /* Tuş */
        int ch;

        /* Çıkış kontrol */
        int shouldExitProgram = 0;

        /* Menüden oyun modu seç */
        gameMode = mode_menu();
        if (gameMode == -1) {
            break;
        }

        /* Zorluk seç */
        difficulty = difficulty_menu();
        if (difficulty == -1) {
            break;
        }

        /* İsimleri moda göre ayarla */
        if (gameMode == MODE_TWO_PLAYER) {
            load_player_names(leftName, rightName, sizeof(leftName));
        } else {
            snprintf(leftName, sizeof(leftName), "Oyuncu");
            snprintf(rightName, sizeof(rightName), "Bot");
        }

        /* Sağ paddle ekranın sağında */
        rightPaddleX = COLS - 3;

        /* İlk servis random */
        reset_ball(&ballX, &ballY, &dirX, &dirY, (rand() % 2 == 0) ? 1 : -1);

        /* Başlangıç hızı */
        delay = get_base_delay(difficulty);

        /* Oyun başında bir kez geri sayım */
        countdown_screen();

        /* Oyun akarken getch beklemesin */
        nodelay(stdscr, TRUE);

        while (1) {
            ch = getch();

            /* Pause menüsü */
            if (ch == 'p' || ch == 'P') {
                int pauseChoice = pause_menu();

                /* Menüden dönünce oyun tekrar akmalı */
                nodelay(stdscr, TRUE);

                if (pauseChoice == -1) {
                    continue;
                } else if (pauseChoice == 0) {
                    /* Devam et */
                } else if (pauseChoice == 1) {
                    /* Ana menüye dön */
                    break;
                } else if (pauseChoice == 2) {
                    /* Programdan çık */
                    shouldExitProgram = 1;
                    break;
                }
            }

            /* Q ile doğrudan çıkış */
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

            /* Sağ paddle X hep sabit */
            rightPaddleX = COLS - 3;

            /* Sağ taraf kontrolü */
            if (gameMode == MODE_BOT) {
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
                /* 2 kişilik mod */
                if (ch == KEY_UP && rightPaddleY > 1) {
                    rightPaddleY--;
                } else if (ch == KEY_DOWN && rightPaddleY + paddleHeight < LINES - 1) {
                    rightPaddleY++;
                }
            }

            /* Top hareketi */
            ballX += dirX;
            ballY += dirY;

            /* Üst-alt duvardan sekme */
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

            /* Sol taraf kaçırdıysa sağ taraf sayı alır */
            if (ballX <= 1) {
                rightScore++;

                /* Dosyaya anlık skoru yaz */
                save_score_to_file(leftName, rightName, leftScore, rightScore);

                /* Kazanma kontrolü */
                if (rightScore >= WIN_SCORE) {
                    int result = winner_screen(rightName);

                    if (result == 0) {
                        /* Ana menüye dön */
                        break;
                    } else {
                        /* Çık */
                        shouldExitProgram = 1;
                        break;
                    }
                }

                /* Hızı ve hit sayısını sıfırla */
                hitCount = 0;
                delay = get_base_delay(difficulty);

                /* Kaybeden taraf sol -> top sola doğru başlar */
                reset_ball(&ballX, &ballY, &dirX, &dirY, -1);

                /* Sadece kısa bekleme */
                usleep(500000);
            }

            /* Sağ taraf kaçırdıysa sol taraf sayı alır */
            if (ballX >= COLS - 2) {
                leftScore++;

                save_score_to_file(leftName, rightName, leftScore, rightScore);

                if (leftScore >= WIN_SCORE) {
                    int result = winner_screen(leftName);

                    if (result == 0) {
                        break;
                    } else {
                        shouldExitProgram = 1;
                        break;
                    }
                }

                hitCount = 0;
                delay = get_base_delay(difficulty);

                /* Kaybeden taraf sağ -> top sağa doğru başlar */
                reset_ball(&ballX, &ballY, &dirX, &dirY, 1);

                usleep(500000);
            }

            /* Ekranı çiz */
            clear();
            box(stdscr, 0, 0);

            /* Orta çizgi */
            for (int y = 1; y < LINES - 1; y++) {
                if (y % 2 == 0) {
                    mvprintw(y, COLS / 2, "|");
                }
            }

            /* Sol paddle */
            for (int i = 0; i < paddleHeight; i++) {
                mvprintw(leftPaddleY + i, leftPaddleX, "|");
            }

            /* Sağ paddle */
            for (int i = 0; i < paddleHeight; i++) {
                mvprintw(rightPaddleY + i, rightPaddleX, "|");
            }

            /* Top */
            mvprintw(ballY, ballX, "O");

            /* Üstte isim + skor */
            mvprintw(1, 3, "%s", leftName);
            mvprintw(1, COLS / 2 - 2, "%d : %d", leftScore, rightScore);
            mvprintw(1, COLS - (int)strlen(rightName) - 4, "%s", rightName);

            /* Bilgi satırı */
            if (gameMode == MODE_BOT) {
                mvprintw(0, COLS / 2 - 30, " W/S hareket | P pause | Mod: Bota karsi | Q cikis ");
            } else {
                mvprintw(0, COLS / 2 - 36, " Sol: W/S | Sag: Yukari/Asagi | P pause | Q cikis ");
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