#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>



/*
    Bu program terminalde ncurses kullanarak Pong benzeri bir oyun oluşturur.

    Eklenen özellikler:
    - Ana menü
    - Oyun modu seçimi
        1 -> Bota karşı
        2 -> 2 kişilik
    - Oyun başlamadan önce geri sayım
    - Skor sistemi
    - Top hızlanması
    - Sağ paddle bot veya oyuncu olabilir

    Kontroller:
    - Sol oyuncu: W / S
    - Sağ oyuncu (2 kişilik modda): Yukarı / Aşağı ok
    - Menüde: 1 veya 2
    - Oyunda çıkış: Q
*/

/* Oyun modlarını daha okunabilir yapmak için sabitler tanımlıyoruz */
#define MODE_BOT 1
#define MODE_TWO_PLAYER 2

/* Oyun başlamadan önce geri sayım göstermek için yardımcı fonksiyon */
void countdown_screen(void) {
    /* 3'ten 1'e kadar saydıracağız */
    for (int i = 3; i >= 1; i--) {
        clear();

        /* Ekranın etrafına çerçeve çiziyoruz */
        box(stdscr, 0, 0);

        /* Geri sayım metinlerini ekranın ortasına yakın yazdırıyoruz */
        mvprintw(LINES / 2 - 1, COLS / 2 - 10, "Oyun basliyor...");
        mvprintw(LINES / 2,     COLS / 2,      "%d", i);

        refresh();

        /* 1 saniye bekletiyoruz */
        sleep(1);
    }

    /* Son olarak "Basla!" yazısını kısa süre gösteriyoruz */
    clear();
    box(stdscr, 0, 0);
    mvprintw(LINES / 2, COLS / 2 - 3, "Basla!");
    refresh();
    usleep(700000); /* 0.7 saniye */
}

/* Menü ekranı:
   Kullanıcıdan oyun modu seçmesini ister.
   1 -> Bot
   2 -> 2 Kişi
*/
int menu_screen(void) {
    int choice;

    /* Menüde tuş beklemek istediğimiz için nodelay kapatıyoruz */
    nodelay(stdscr, FALSE);

    while (1) {
        clear();
        box(stdscr, 0, 0);

        /* Menü başlığı */
        mvprintw(LINES / 2 - 4, COLS / 2 - 7, "PONG MENU");

        /* Menü seçenekleri */
        mvprintw(LINES / 2 - 1, COLS / 2 - 16, "1 - Bota karsi oyna");
        mvprintw(LINES / 2,     COLS / 2 - 16, "2 - 2 kisilik oyna");

        /* Kontrol bilgisi */
        mvprintw(LINES / 2 + 2, COLS / 2 - 20, "Secim yapmak icin 1 veya 2 tusla");
        mvprintw(LINES / 2 + 4, COLS / 2 - 24, "Sol oyuncu: W/S   Sag oyuncu: Yukari/Asagi");
        mvprintw(LINES / 2 + 5, COLS / 2 - 10, "Cikis: Q");

        refresh();

        choice = getch();

        /* Kullanıcı Q basarsa program sonlansın diye -1 döndürüyoruz */
        if (choice == 'q' || choice == 'Q') {
            return -1;
        }

        /* 1 veya 2 basılırsa ilgili modu döndürüyoruz */
        if (choice == '1') {
            return MODE_BOT;
        } else if (choice == '2') {
            return MODE_TWO_PLAYER;
        }
    }
}

int main(void) {
    /* Topun pozisyonu */
    int ballX, ballY;

    /* Topun yönü:
       dirX = 1 ise sağa, -1 ise sola
       dirY = 1 ise aşağı, -1 ise yukarı
    */
    int dirX, dirY;

    /* Sol paddle'ın X sabit, Y değişken */
    int leftPaddleX = 2;
    int leftPaddleY = 8;

    /* Sağ paddle'ın X sabit, Y değişken */
    int rightPaddleX;
    int rightPaddleY = 8;

    /* Paddle yüksekliği */
    int paddleHeight = 4;

    /* Skor değişkenleri */
    int leftScore = 0;
    int rightScore = 0;

    /* Top gecikmesi:
       Küçüldükçe top daha hızlı hareket eder
    */
    int delay = 50000;

    /* Klavyeden alınan tuş */
    int ch;

    /* Seçilen oyun modu burada tutulacak */
    int gameMode;

    /* ncurses başlatılır */
    initscr();

    srand(time(NULL));  // random seed

    /* Kullanıcının bastığı tuşlar ekranda görünmesin */
    noecho();

    /* Karakter bazlı giriş modu */
    cbreak();

    /* İmleç görünmesin */
    curs_set(0);

    /* Özel tuşları (ok tuşları gibi) okuyabilmek için */
    keypad(stdscr, TRUE);

    /* Menüye gitmeden önce sağ paddle X konumunu ayarlıyoruz */
    rightPaddleX = COLS - 3;

    /* Önce menüyü gösteriyoruz */
    gameMode = menu_screen();

    /* Kullanıcı menüde Q ile çıktıysa program kapanır */
    if (gameMode == -1) {
        endwin();
        return 0;
    }

    /* Menüden sonra oyun başlamadan geri sayım göster */
    countdown_screen();

    /* Oyun sırasında sürekli tuş okuyabilmek için nodelay açıyoruz
       Böylece getch() beklemez, oyun akmaya devam eder
    */
    nodelay(stdscr, TRUE);

    /* Topu ekranın ortasında başlat */
    ballX = COLS / 2;
    ballY = LINES / 2;

    /* Başlangıçta top sağa ve aşağı hareket etsin */
    dirX = (rand() % 2 == 0) ? 1 : -1;
    dirY = (rand() % 2 == 0) ? 1 : -1;

    /* Ana oyun döngüsü */
    while (1) {
        /* Kullanıcıdan tuş oku */
        ch = getch();

        /* Q basılırsa oyundan çık */
        if (ch == 'q' || ch == 'Q') {
            break;
        }

        /* ---------------- SOL OYUNCU KONTROLÜ ----------------
           Sol paddle W ve S ile hareket eder
        */
        if ((ch == 'w' || ch == 'W') && leftPaddleY > 1) {
            leftPaddleY--;
        } else if ((ch == 's' || ch == 'S') && leftPaddleY + paddleHeight < LINES - 1) {
            leftPaddleY++;
        }

        /* Sağ paddle her frame ekrana göre tekrar ayarlansın */
        rightPaddleX = COLS - 3;

        /* ---------------- SAG TARAF KONTROLÜ ----------------
           Eğer mod bot ise bot topu takip eder
           Eğer mod 2 kişilik ise sağ oyuncu ok tuşlarıyla oynar
        */
        if (gameMode == MODE_BOT) {
            /* Sağ paddle'ın orta noktası */
            int rightCenter = rightPaddleY + paddleHeight / 2;

            /* Top paddle merkezinden yukarıdaysa yukarı çık */
            if (ballY < rightCenter && rightPaddleY > 1) {
                rightPaddleY--;
            }
            /* Top paddle merkezinden aşağıdaysa aşağı in */
            else if (ballY > rightCenter && rightPaddleY + paddleHeight < LINES - 1) {
                rightPaddleY++;
            }
        } else if (gameMode == MODE_TWO_PLAYER) {
            /* 2 kişilikte sağ oyuncu ok tuşlarıyla oynar */
            if (ch == KEY_UP && rightPaddleY > 1) {
                rightPaddleY--;
            } else if (ch == KEY_DOWN && rightPaddleY + paddleHeight < LINES - 1) {
                rightPaddleY++;
            }
        }

        /* ---------------- TOP HAREKETI ---------------- */
        ballX += dirX;
        ballY += dirY;

        /* ---------------- DUVAR CARPISMASI ----------------
           Top üst veya alt duvara çarparsa Y yönü ters döner
        */
        if (ballY <= 1) {
            ballY = 1;
            dirY = 1;
        } else if (ballY >= LINES - 2) {
            ballY = LINES - 2;
            dirY = -1;
        }

        /* ---------------- SOL PADDLE CARPISMASI ----------------
           Top, sol paddle'ın hemen sağ kenarına geldiğinde ve
           Y olarak paddle aralığındaysa çarpışma olmuş olur
        */
        if (ballX == leftPaddleX + 1 &&
            ballY >= leftPaddleY &&
            ballY < leftPaddleY + paddleHeight) {

            /* Topu sağa geri gönder */
            dirX = 1;

            /* Top paddle'ın neresine vurduysa ona göre
               yukarı / aşağı yönünü değiştiriyoruz
            */
            int hitPos = ballY - leftPaddleY;

            if (hitPos == 0) {
                dirY = -1; /* üst tarafa vurduysa yukarı */
            } else if (hitPos == paddleHeight - 1) {
                dirY = 1;  /* alt tarafa vurduysa aşağı */
            }

            /* Her çarpışmada biraz hızlandır */
            if (delay > 15000) {
                delay -= 2000;
            }
        }

        /* ---------------- SAG PADDLE CARPISMASI ---------------- */
        if (ballX == rightPaddleX - 1 &&
            ballY >= rightPaddleY &&
            ballY < rightPaddleY + paddleHeight) {

            /* Topu sola geri gönder */
            dirX = -1;

            int hitPos = ballY - rightPaddleY;

            if (hitPos == 0) {
                dirY = -1;
            } else if (hitPos == paddleHeight - 1) {
                dirY = 1;
            }

            if (delay > 15000) {
                delay -= 2000;
            }
        }

        /* ---------------- SKOR KONTROLU ----------------
           Top sol duvarı geçtiyse sağ taraf sayı alır
        */
        if (ballX <= 1) {
            rightScore++;

            /* Topu merkeze sıfırla */
            ballX = COLS / 2;
            ballY = LINES / 2;

            /* Sonraki servis sağa gitsin */
            dirX = -1;
            dirY = rand() % 2 == 0 ? 1 : -1;

            /* Hızı başa al */
            delay = 50000;

            /* Sayıdan sonra kısa geri sayım */
            countdown_screen();
        }

        /* Top sağ duvarı geçtiyse sol taraf sayı alır */
        if (ballX >= COLS - 2) {
            leftScore++;

            ballX = COLS / 2;
            ballY = LINES / 2;

            /* Sonraki servis sola gitsin */
            dirX = 1;
            dirY = rand() % 2 == 0 ? 1 : -1;

            delay = 50000;

            countdown_screen();
        }

        /* ---------------- EKRANI CIZME ---------------- */
        clear();

        /* Dış çerçeve */
        box(stdscr, 0, 0);

        /* Orta çizgi */
        for (int y = 1; y < LINES - 1; y++) {
            if (y % 2 == 0) {
                mvprintw(y, COLS / 2, "|");
            }
        }

        /* Sol paddle'ı çiz */
        for (int i = 0; i < paddleHeight; i++) {
            mvprintw(leftPaddleY + i, leftPaddleX, "|");
        }

        /* Sağ paddle'ı çiz */
        for (int i = 0; i < paddleHeight; i++) {
            mvprintw(rightPaddleY + i, rightPaddleX, "|");
        }

        /* Topu çiz */
        mvprintw(ballY, ballX, "O");

        /* Ekranın üst kısmına skor yaz */
        mvprintw(1, COLS / 2 - 2, "%d : %d", leftScore, rightScore);

        /* Oyun moduna göre bilgi metni */
        if (gameMode == MODE_BOT) {
            mvprintw(0, COLS / 2 - 20, " W/S hareket  |  Mod: Bota karsi  |  Q cikis ");
        } else {
            mvprintw(0, COLS / 2 - 28, " Sol: W/S  |  Sag: Yukari/Asagi  |  Mod: 2 kisilik  |  Q cikis ");
        }

        /* Ekranda yapılan değişiklikleri gerçekten göster */
        refresh();

        /* Topun hızını kontrol eden küçük bekleme */
        usleep(delay);
    }

    /* ncurses modunu kapatıp terminali normale döndür */
    endwin();
    return 0;
}