#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_X 18
#define BUTTON_Y 19
#define BUTTON_P 23

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define HALF_SCREEN_WIDTH (SCREEN_WIDTH / 2)
#define HALF_SCREEN_HEIGHT (SCREEN_HEIGHT / 2)

#define RECTANGLE_COUNT 9
#define RESET_DELAY 3000

bool bx_lastbstate = HIGH;
bool by_lastbstate = HIGH;
bool bp_lastbstate = HIGH;

u8_t winner_slot_a = 0;
u8_t winner_slot_b = 0;

u8_t selected_rect = 0;
char board[3][3] = { {'N', 'N', 'N'}, {'N', 'N', 'N'}, {'N', 'N', 'N'} };
bool current_player = true;
bool is_winner = false;
bool is_draw = false;

u64_t reset_time = 0;
bool game_ended = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

typedef struct Rectangle {
    u8_t x, y, w, h;
} Rectangle;

Rectangle rectangles[RECTANGLE_COUNT] = { };

void setup_rectangles() {
    u8_t rectangle_width = HALF_SCREEN_WIDTH / 3;
    u8_t rectangle_height = SCREEN_HEIGHT / 3;

    for (u8_t row = 0; row < 3; row++) {
        for (u8_t column = 0; column < 3; column++) {
            rectangles[row * 3 + column] = { column * rectangle_width, row * rectangle_height, rectangle_width, rectangle_height };
        }
    }
}

bool read_button(u8_t pin, bool &last_button_state) {
    bool button_state = !digitalRead(pin);
    bool pressed = button_state != last_button_state && button_state;

    last_button_state = button_state;
    return pressed;
}

bool check_line(char a, char b, char c) {
    return a != 'N' && a == b && b == c;
}

u8_t matrix_to_index(u8_t row, u8_t column) {
    return row * 3 + column;
}

char check_winner(u8_t &a, u8_t &b) {
    for (u8_t t = 0; t < 3; t++) {
        if (check_line(board[t][0], board[t][1], board[t][2])) {
            winner_slot_a = matrix_to_index(t, 0);
            winner_slot_b = matrix_to_index(t, 2);
            return board[t][0];
        }

        if (check_line(board[0][t], board[1][t], board[2][t])) {
            winner_slot_a = matrix_to_index(0, t);
            winner_slot_b = matrix_to_index(2, t);
            return board[0][t];
        }
    }

    if (check_line(board[0][0], board[1][1], board[2][2])) {
        winner_slot_a = matrix_to_index(0, 0);
        winner_slot_b = matrix_to_index(2, 2);
        return board[0][0];
    }

    if (check_line(board[0][2], board[1][1], board[2][0])) {
        winner_slot_a = matrix_to_index(0, 2);
        winner_slot_b = matrix_to_index(2, 0);
        return board[0][2];
    }

    return 'N';
}

bool check_draw() {
    for (u8_t a = 0; a < 3; a++) {
        for (u8_t b = 0; b < 3; b++) {
            if (board[a][b] == 'N') return false;
        }
    }

    return true;
}

void draw_board() {
    for (u8_t t = 0; t < RECTANGLE_COUNT; t++) {
        Rectangle rectangle = rectangles[t];
        display.drawRect(rectangle.x, rectangle.y, rectangle.w, rectangle.h, SSD1306_WHITE);
        if (t == selected_rect) {
            display.drawRect(rectangle.x + 1, rectangle.y + 1, rectangle.w - 2, rectangle.h - 2, SSD1306_WHITE);
        }

        u8_t row = t / 3;
        u8_t column = t % 3;
        u8_t pos_x = rectangle.x + (rectangle.w / 2) - 2;
        u8_t pos_y = rectangle.y + (rectangle.h / 2) - 3;

        char symbol = board[row][column];
        if (symbol != 'N') {
            display.setCursor(pos_x, pos_y);
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.print(symbol);
        }
    }
}

void draw_winner(char winner) {
    display.setCursor(HALF_SCREEN_WIDTH + 13, 7);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Winner:");

    display.setCursor(HALF_SCREEN_WIDTH + HALF_SCREEN_WIDTH / 2 - 3, HALF_SCREEN_HEIGHT);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.print(winner);
}

void draw_line(u8_t a, u8_t b) {
    Rectangle rec_a = rectangles[a];
    Rectangle rec_b = rectangles[b];

    u8_t rec_a_x = rec_a.x + (rec_a.w / 2);
    u8_t rec_a_y = rec_a.y + (rec_a.h / 2);
    u8_t rec_b_x = rec_b.x + (rec_b.w / 2);
    u8_t rec_b_y = rec_b.y + (rec_b.h / 2);

    display.drawLine(rec_a_x, rec_a_y, rec_b_x, rec_b_y, SSD1306_WHITE);
}

void draw_draw() {
    display.setCursor(HALF_SCREEN_WIDTH + 21, 7);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Draw!");

    display.setCursor(HALF_SCREEN_WIDTH + HALF_SCREEN_WIDTH / 2 - 13, HALF_SCREEN_HEIGHT);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.print(":3");
}

void draw_current_player() {
    display.setCursor(HALF_SCREEN_WIDTH + 11, 7);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("Current:");

    display.setCursor(HALF_SCREEN_WIDTH + HALF_SCREEN_WIDTH / 2 - 3, HALF_SCREEN_HEIGHT);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    char player_symbol = current_player ? 'X' : 'O';
    display.print(player_symbol);
}

void reset_game() {
    for (u8_t a = 0; a < 3; a++) {
        for (u8_t b = 0; b < 3; b++) {
            board[a][b] = 'N';
        }
    }

    selected_rect = 0;
    current_player = true;
    is_winner = false;
    is_draw = false;

    reset_time = 0;
    game_ended = false;
}

void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_X, INPUT_PULLUP);
    pinMode(BUTTON_Y, INPUT_PULLUP);
    pinMode(BUTTON_P, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed, restarting..."));

        delay(7000);
        ESP.restart();
    }

    setup_rectangles();
}

void loop() {
    display.clearDisplay();

    bool bx = read_button(BUTTON_X, bx_lastbstate);
    bool by = read_button(BUTTON_Y, by_lastbstate);
    bool bp = read_button(BUTTON_P, bp_lastbstate);

    char winner = check_winner(winner_slot_a, winner_slot_b);
    is_winner = (winner != 'N');
    is_draw = check_draw() && !is_winner;

    if (bx) {
        if (selected_rect % 3 < 2) {
            selected_rect++;
        } else {
            selected_rect -= 2;
        }
    }

    if (by) {
        if (selected_rect < 6) {
            selected_rect += 3;
        } else {
            selected_rect -= 6;
        }
    }

    if (bp && !is_winner && !is_draw) {
        u8_t row = selected_rect / 3;
        u8_t column = selected_rect % 3;

        if (board[row][column] == 'N') {
            board[row][column] = current_player ? 'X' : 'O';
            current_player = !current_player;
        }
    }

    if (is_winner || is_draw) {
        if (!game_ended) {
            reset_time = millis();
            game_ended = true;
        }

        draw_board();
        if (is_winner) {
            draw_winner(winner);
            draw_line(winner_slot_a, winner_slot_b);
        } else if (is_draw) {
            draw_draw();
        }
    } else {
        draw_board();
        draw_current_player();
    }

    if (game_ended && millis() - reset_time >= RESET_DELAY) {
        reset_game();
    }

    display.display();
}
