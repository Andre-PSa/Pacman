#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    char lvlname[32];
    char pacname[32];
    char monname[5][32];  // Array for multiple monster files

    sprintf(lvlname, "test%d.lvl", n);
    sprintf(pacname, "test%d.p", n);

    /* ================================
       Create LEVEL FILE
       ================================ */
    FILE *lvl = fopen(lvlname, "w");
    if (!lvl) { perror("Cannot create lvl file"); return 1; }

    // Different levels based on n
    switch(n) {
        case 1:
            sprintf(monname[0], "test%d_m1.m", n);
            fprintf(lvl,
                "DIM 8 8\n"
                "TEMPO 10\n"
                "PAC %s\n"
                "MON %s\n"
                "XXXXXXXX\n"
                "XooooooX\n"
                "XXXoXXoX\n"
                "XooooXoX\n"
                "XoXXXXoX\n"
                "Xooooooo\n"
                "XXXoXXo@\n"
                "XXXXXXXX",
                pacname, monname[0]
            );
            break;

        case 2:
            sprintf(monname[0], "test%d_m1.m", n);
            sprintf(monname[1], "test%d_m2.m", n);
            fprintf(lvl,
                "DIM 10 10\n"
                "TEMPO 10\n"
                "PAC %s\n"
                "MON %s\n"
                "MON %s\n"
                "XXXXXXXXXX\n"
                "XoooooXooX\n"
                "XXXoXXXooX\n"
                "XooooooooX\n"
                "XoXXXXXXoX\n"
                "XoXooooXoX\n"
                "XoXoXXoXoX\n"
                "XoooXooooX\n"
                "XXXXXXXo@X\n"
                "XXXXXXXXXX",
                pacname, monname[0], monname[1]
            );
            break;

        case 3:
            sprintf(monname[0], "test%d_m1.m", n);
            sprintf(monname[1], "test%d_m2.m", n);
            sprintf(monname[2], "test%d_m3.m", n);
            fprintf(lvl,
                "DIM 12 8\n"
                "TEMPO 10\n"
                "PAC %s\n"
                "MON %s\n"
                "MON %s\n"
                "MON %s\n"
                "XXXXXXXXXXXX\n"
                "XooooXXXoooX\n"
                "XXXoXoooXXoX\n"
                "XooooXXooooo\n"
                "XoXXXXXXXXoX\n"
                "XoooooooooX@\n"
                "XXXXoXXXXXXX\n"
                "XXXXXXXXXXXX",
                pacname, monname[0], monname[1], monname[2]
            );
            break;

        case 4:
            sprintf(monname[0], "test%d_m1.m", n);
            sprintf(monname[1], "test%d_m2.m", n);
            sprintf(monname[2], "test%d_m3.m", n);
            sprintf(monname[3], "test%d_m4.m", n);
            fprintf(lvl,
                "DIM 14 10\n"
                "TEMPO 10\n"
                "PAC %s\n"
                "MON %s\n"
                "MON %s\n"
                "MON %s\n"
                "MON %s\n"
                "XXXXXXXXXXXXXX\n"
                "XooooXXXXXoooX\n"
                "XXXoXooooXXXoX\n"
                "XooooXXXoooooX\n"
                "XoXXXXXXXXXXoX\n"
                "XoXoooooooXooX\n"
                "XoXoXXXXXoXXoX\n"
                "XoooXooooooooX\n"
                "XXXXXXXXXXXo@X\n"
                "XXXXXXXXXXXXXX",
                pacname, monname[0], monname[1], monname[2], monname[3]
            );
            break;

        case 5:
            sprintf(monname[0], "test%d_m1.m", n);
            sprintf(monname[1], "test%d_m2.m", n);
            sprintf(monname[2], "test%d_m3.m", n);
            sprintf(monname[3], "test%d_m4.m", n);
            sprintf(monname[4], "test%d_m5.m", n);
            fprintf(lvl,
                "DIM 16 12\n"
                "TEMPO 10\n"
                "PAC %s\n"
                "MON %s\n"
                "MON %s\n"
                "MON %s\n"
                "MON %s\n"
                "MON %s\n"
                "XXXXXXXXXXXXXXXX\n"
                "XooooXXXXXXXoooX\n"
                "XXXoXoooooooXXoX\n"
                "XooooXXXXXXooooo\n"
                "XoXXXXXXXXXXXXoX\n"
                "XoXooooooooooXoX\n"
                "XoXoXXXXXXXXoXoX\n"
                "XoooXoooooooXooX\n"
                "XXXXXXXXXoXXXXoX\n"
                "XooooooooooooooX\n"
                "XXXXXXXXXXXXXo@X\n"
                "XXXXXXXXXXXXXXXX",
                pacname, monname[0], monname[1], monname[2], monname[3], monname[4]
            );
            break;

        default:
            sprintf(monname[0], "test%d_m1.m", n);
            fprintf(lvl,
                "DIM 6 6\n"
                "TEMPO 10\n"
                "PAC %s\n"
                "MON %s\n"
                "XXoXXX\n"
                "XoooXX\n"
                "XoXoXX\n"
                "Xooo@X\n"
                "XooooX\n"
                "XXXXXX",
                pacname, monname[0]
            );
    }

    fclose(lvl);

    /* ================================
       Create PACMAN FILE (empty for user input)
       ================================ */
    FILE *pac = fopen(pacname, "w");
    if (!pac) { perror("Cannot create pac file"); return 1; }
    fprintf(pac, "PASSO 0\nPOS 1 1\n");
    fclose(pac);

    /* ================================
       Create MONSTER FILES
       ================================ */
    
    // Helper function to write monster file without trailing newline
    void write_monster_file(const char* filename, const char* content) {
        FILE* f = fopen(filename, "w");
        if (!f) { 
            perror("Cannot create monster file"); 
            exit(1); 
        }
        // Write content but remove trailing newline if present
        int len = strlen(content);
        if (len > 0 && content[len-1] == '\n') {
            fwrite(content, 1, len - 1, f);
        } else {
            fputs(content, f);
        }
        fclose(f);
    }

    // Different ghost behaviors per level
    switch(n) {
        case 1:
            // Simple patrol ghost - medium speed
            write_monster_file(monname[0],
                "PASSO 25\n"
                "POS 5 1\n"
                "A\n"
                "A\n"
                "A\n"
                "D\n"
                "D\n"
                "D"
            );
            break;

        case 3:
            // Three ghosts with different speeds - slow, medium, fast
            write_monster_file(monname[0],
                "PASSO 30\n"
                "POS 8 1\n"
                "A\n"
                "A\n"
                "S\n"
                "D\n"
                "D\n"
                "W"
            );
            write_monster_file(monname[1],
                "PASSO 25\n"
                "POS 5 3\n"
                "S\n"
                "S\n"
                "A\n"
                "W\n"
                "W\n"
                "D"
            );
            write_monster_file(monname[2],
                "PASSO 20\n"
                "POS 10 5\n"
                "A\n"
                "A\n"
                "A\n"
                "W\n"
                "D\n"
                "D\n"
                "D\n"
                "S"
            );
            break;

        case 4:
            // Four ghosts - mixed speeds with charge moves
            write_monster_file(monname[0],
                "PASSO 25\n"
                "POS 10 1\n"
                "A\n"
                "S\n"
                "C\n"
                "D"
            );
            write_monster_file(monname[1],
                "PASSO 30\n"
                "POS 5 4\n"
                "W\n"
                "W\n"
                "C\n"
                "A\n"
                "S\n"
                "S"
            );
            write_monster_file(monname[2],
                "PASSO 25\n"
                "POS 8 6\n"
                "D\n"
                "D\n"
                "W\n"
                "C\n"
                "S\n"
                "A\n"
                "A"
            );
            write_monster_file(monname[3],
                "PASSO 20\n"
                "POS 2 7\n"
                "D\n"
                "C\n"
                "W\n"
                "A\n"
                "S\n"
                "D"
            );
            break;

        case 5:
            // Five aggressive ghosts with charges and waits
            write_monster_file(monname[0],
                "PASSO 30\n"
                "POS 12 1\n"
                "A\n"
                "C\n"
                "S\n"
                "D"
            );
            write_monster_file(monname[1],
                "PASSO 25\n"
                "POS 6 5\n"
                "T 2\n"
                "C\n"
                "A\n"
                "S\n"
                "D"
            );
            write_monster_file(monname[2],
                "PASSO 25\n"
                "POS 10 5\n"
                "W\n"
                "C\n"
                "D\n"
                "S\n"
                "A"
            );
            write_monster_file(monname[3],
                "PASSO 20\n"
                "POS 3 7\n"
                "D\n"
                "D\n"
                "C\n"
                "W\n"
                "A\n"
                "S"
            );
            write_monster_file(monname[4],
                "PASSO 25\n"
                "POS 13 9\n"
                "A\n"
                "A\n"
                "W\n"
                "C\n"
                "S\n"
                "D"
            );
            break;

        default:
            write_monster_file(monname[0],
                "PASSO 25\n"
                "POS 2 1\n"
                "A\n"
                "D\n"
                "D\n"
                "T 3\n"
                "W\n"
                "S"
            );
            break;

        case 2:
            // Two ghosts - one medium, one fast
            write_monster_file(monname[0],
                "PASSO 25\n"
                "POS 7 1\n"
                "A\n"
                "S\n"
                "D\n"
                "W"
            );
            write_monster_file(monname[1],
                "PASSO 20\n"
                "POS 4 4\n"
                "W\n"
                "W\n"
                "A\n"
                "A\n"
                "S\n"
                "S");
    }

    printf("Generated: %s, %s", lvlname, pacname);
    for (int i = 0; i < (n <= 5 ? n : 1); i++) {
        printf(", %s", monname[i]);
    }
    printf("\n");
    return 0;
}