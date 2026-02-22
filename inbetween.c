#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

static void trim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) {
        s[n-1] = '\0';
        n--;
    }
    size_t i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
}

static void upper_str(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

static void init_deck(bool in_deck[52]) {
    for (int i = 0; i < 52; i++) in_deck[i] = true;
}

// rank: A=1, 2-10, J=11, Q=12, K=13
// suit: C D H S
// input examples: AS, 10H, QD, TC
static int parse_card(const char *in, int *rank, int *suit, int *card_id) {
    char buf[32];
    strncpy(buf, in, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    trim(buf);
    if (buf[0] == '\0') return 0;

    upper_str(buf);
    size_t len = strlen(buf);
    if (len < 2 || len > 3) return 0;

    char suit_ch = buf[len - 1];
    int s;
    switch (suit_ch) {
        case 'C': s = 0; break;
        case 'D': s = 1; break;
        case 'H': s = 2; break;
        case 'S': s = 3; break;
        default: return 0;
    }

    char rtok[8];
    memcpy(rtok, buf, len - 1);
    rtok[len - 1] = '\0';

    int r = 0;
    if (strcmp(rtok, "A") == 0) r = 1;
    else if (strcmp(rtok, "J") == 0) r = 11;
    else if (strcmp(rtok, "Q") == 0) r = 12;
    else if (strcmp(rtok, "K") == 0) r = 13;
    else if (strcmp(rtok, "T") == 0) r = 10;
    else {
        char *endp = NULL;
        long val = strtol(rtok, &endp, 10);
        if (!endp || *endp != '\0') return 0;
        if (val < 2 || val > 10) return 0;
        r = (int)val;
    }

    int id = s * 13 + (r - 1);
    if (rank) *rank = r;
    if (suit) *suit = s;
    if (card_id) *card_id = id;
    return 1;
}

static const char* rank_name(int r) {
    static char buf[4];
    if (r == 1)  return "A";
    if (r == 11) return "J";
    if (r == 12) return "Q";
    if (r == 13) return "K";
    snprintf(buf, sizeof(buf), "%d", r);
    return buf;
}

static int remaining_counts(const bool in_deck[52], int rankCount[14]) {
    for (int r = 0; r <= 13; r++) rankCount[r] = 0;
    int rem = 0;
    for (int id = 0; id < 52; id++) {
        if (in_deck[id]) {
            int r = (id % 13) + 1;
            rankCount[r]++;
            rem++;
        }
    }
    return rem;
}

static int read_card_remove(const char *prompt, bool in_deck[52], int *out_rank, int *out_id) {
    char line[64];
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) return 0;
        trim(line);
        if (line[0] == '\0') continue;

        int r, s, id;
        if (!parse_card(line, &r, &s, &id)) {
            printf("  Invalid format. Examples: AS, 10H, QD, KC (suits C/D/H/S).\n");
            continue;
        }
        if (!in_deck[id]) {
            printf("  That card has already been removed (duplicate / already used).\n");
            continue;
        }
        in_deck[id] = false;
        if (out_rank) *out_rank = r;
        if (out_id) *out_id = id;
        return 1;
    }
}

static int read_big_small(void) {
    char line[64];
    while (1) {
        printf("Pair gate: choose BIG (>) or SMALL (<): ");
        if (!fgets(line, sizeof(line), stdin)) return 1;
        trim(line);
        if (line[0] == '\0') continue;

        upper_str(line);
        if (strcmp(line, "BIG") == 0 || strcmp(line, "B") == 0) return 1;
        if (strcmp(line, "SMALL") == 0 || strcmp(line, "S") == 0) return 0;

        printf("  Please type BIG or SMALL (or B / S).\n");
    }
}

static void input_seen_cards(bool deck[52]) {
    char line[64];
    while (1) {
        printf("Seen card (DONE to stop): ");
        if (!fgets(line, sizeof(line), stdin)) break;
        trim(line);
        if (line[0] == '\0') continue;

        char tmp[64];
        strncpy(tmp, line, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        upper_str(tmp);

        if (strcmp(tmp, "DONE") == 0 || strcmp(tmp, "END") == 0) break;

        int r, s, id;
        if (!parse_card(line, &r, &s, &id)) {
            printf("  Invalid format. Examples: AS, 10H, QD.\n");
            continue;
        }
        if (!deck[id]) {
            printf("  This card is already removed (duplicate).\n");
            continue;
        }
        deck[id] = false;
    }
}

static void print_nonpair_result(int low, int high, int winCount, int edgeCount, int loseCount, int rem) {
    double pWin  = (double)winCount  / rem;
    double pEdge = (double)edgeCount / rem;
    double pLose = (double)loseCount / rem;

    // Stake=1: win +1, outside -1, edge -2
    double ev = ((double)winCount * 1.0 + (double)loseCount * (-1.0) + (double)edgeCount * (-2.0)) / rem;

    printf("\n--- Result (Non-pair gate: bet IN-BETWEEN) ---\n");
    printf("Between range: (%s, %s)\n", rank_name(low), rank_name(high));
    printf("Win (in-between): %d / %d = %.2f%%\n", winCount, rem, pWin * 100.0);
    printf("Edge (lose 2x):   %d / %d = %.2f%%\n", edgeCount, rem, pEdge * 100.0);
    printf("Outside (lose 1x):%d / %d = %.2f%%\n", loseCount, rem, pLose * 100.0);
    printf("EV (per 1 unit bet): %.4f\n", ev);

    if (high == low + 1) {
        printf("Note: Adjacent gate -> no ranks in-between, Win rate = 0%%.\n");
    }
}

static void print_pair_result(int gate, int bigWin, int smallWin, int tripleCount,
                             int winCount, int loseCount, int rem, int guessBig) {
    double pWin    = (double)winCount / rem;
    double pTriple = (double)tripleCount / rem;
    double pLose   = (double)loseCount / rem;

    // Stake=1: win +1, normal lose -1, triple -3
    double ev = ((double)winCount * 1.0 + (double)loseCount * (-1.0) + (double)tripleCount * (-3.0)) / rem;

    
    printf("Gate rank: %s\n", rank_name(gate));
    printf("If BIG (>):   %d/%d = %.2f%%\n", bigWin, rem, (double)bigWin * 100.0 / rem);
    printf("If SMALL (<): %d/%d = %.2f%%\n", smallWin, rem, (double)smallWin * 100.0 / rem);
    printf("Triple same (=, lose 3x): %d/%d = %.2f%%\n", tripleCount, rem, pTriple * 100.0);

    printf("\nYou chose: %s\n", guessBig ? "BIG" : "SMALL");
    printf("Win:                %d/%d = %.2f%%\n", winCount, rem, pWin * 100.0);
    printf("Normal lose (1x):    %d/%d = %.2f%%\n", loseCount, rem, pLose * 100.0);
    printf("Triple lose (3x):    %d/%d = %.2f%%\n", tripleCount, rem, pTriple * 100.0);
    printf("EV (per 1 unit bet): %.4f\n", ev);

    if (bigWin > smallWin) printf("Suggestion: BIG has higher win rate.\n");
    else if (smallWin > bigWin) printf("Suggestion: SMALL has higher win rate.\n");
    else printf("Suggestion: BIG and SMALL have the same win rate.\n");
}

static void run_one_round(bool deck[52]) {
    int rankCount[14];
    int rem_before = remaining_counts(deck, rankCount);
    if (rem_before < 2) {
        printf("Not enough cards left to draw a gate.\n");
        return;
    }

    printf("\n========== New Round ==========\n");
    printf("Remaining drawable cards (before gate): %d\n", rem_before);

    // Gate cards are removed from MAIN deck immediately
    int r1=0, r2=0;
    if (!read_card_remove("Enter Gate Card 1: ", deck, &r1, NULL)) return;
    if (!read_card_remove("Enter Gate Card 2: ", deck, &r2, NULL)) return;

    int rem = remaining_counts(deck, rankCount);
    printf("Gate: %s and %s (Remaining after gate removed: %d)\n", rank_name(r1), rank_name(r2), rem);

    if (rem <= 0) {
        printf("No cards left to draw.\n");
        return;
    }

    if (r1 != r2) {
        int low  = (r1 < r2) ? r1 : r2;
        int high = (r1 < r2) ? r2 : r1;

        int winCount = 0;
        for (int r = low + 1; r <= high - 1; r++) winCount += rankCount[r];

        int edgeCount = rankCount[low] + rankCount[high];
        int loseCount = rem - winCount - edgeCount;

        print_nonpair_result(low, high, winCount, edgeCount, loseCount, rem);
    } else {
        int gate = r1;
        int guessBig = read_big_small();

        int tripleCount = rankCount[gate]; // remaining same rank (0..2)
        int bigWin = 0, smallWin = 0;
        for (int r = gate + 1; r <= 13; r++) bigWin += rankCount[r];
        for (int r = 1; r <= gate - 1; r++) smallWin += rankCount[r];

        int winCount  = guessBig ? bigWin : smallWin;
        int loseCount = rem - winCount - tripleCount;

        print_pair_result(gate, bigWin, smallWin, tripleCount, winCount, loseCount, rem, guessBig);
    }

    // Third card removal
    char line[64];
    while (1) {
        printf("\nEnter the revealed 3rd card to remove it from the deck.\n");
        printf("If not revealed yet, type SKIP (you can remove it later via ADD): ");
        if (!fgets(line, sizeof(line), stdin)) break;
        trim(line);
        if (line[0] == '\0') continue;

        char tmp[64];
        strncpy(tmp, line, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        upper_str(tmp);

        if (strcmp(tmp, "SKIP") == 0) {
            printf("use ADD later to remove the 3rd card when it is revealed.\n");
            break;
        }

        int r, s, id;
        if (!parse_card(line, &r, &s, &id)) {
            printf("  Invalid format. Examples: 7S, AH, 10D.\n");
            continue;
        }
        if (!deck[id]) {
            printf("  That card is not available in the remaining deck (already removed / wrong input).\n");
            continue;
        }
        deck[id] = false;
        printf("  Updated: 3rd card %s removed from deck.\n", line);
        break;
    }

    int rem_after = remaining_counts(deck, rankCount);
    printf("Deck updated. Remaining drawable cards now: %d\n", rem_after);
}

int main(void) {
    bool deck[52];
    init_deck(deck);

    printf("Card format: AS, 10H, QD, KC (suits: C/D/H/S). 'T' also works for 10 (e.g., TS).\n");

    input_seen_cards(deck);

    while (1) {
        int rankCount[14];
        int rem = remaining_counts(deck, rankCount);
        if (rem < 2) {
            printf("\nNot enough cards left to continue.\n");
            break;
        }

        run_one_round(deck);

        char cmd[64];
        printf("\nNext: ENTER=continue / ADD=add more seen cards / NEW=reset new deck / QUIT=exit > ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        trim(cmd);

        if (cmd[0] == '\0') continue;

        char u[64];
        strncpy(u, cmd, sizeof(u)-1);
        u[sizeof(u)-1] = '\0';
        upper_str(u);

        if (strcmp(u, "QUIT") == 0 || strcmp(u, "Q") == 0) break;
        if (strcmp(u, "ADD") == 0) {
            input_seen_cards(deck);
            continue;
        }
        if (strcmp(u, "NEW") == 0) {
            init_deck(deck);
            input_seen_cards(deck);
            continue;
        }
        // anything else -> continue
    }

    printf("Done.\n");
    return 0;
}