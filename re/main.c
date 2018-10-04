#include <stdio.h>
#include <string.h>
#include "re2nfa.h"
#include "nfa2dfa.h"
#include "dfa_complement.h"
#include "dfa2re.h"

int
main()
{
    fprintf(stderr, "Regex: ");
    char re[1024];
    if (scanf("%1023s", re) != 1) return 1;

    fprintf(stderr, "Alphabet: ");
    char alphabet[1024];
    if (scanf("%1023s", alphabet) != 1) return 1;

    NFA nfa = re2nfa(re, re + strlen(re));
    if (nfa_has_error(nfa)) {
        puts("Invalid regex!");
        return 1;
    }
    puts("Built NFA:");
    for (size_t i = 0; i < nfa.states.size; ++i) {
        NFA_State *s = &nfa.states.data[i];
        printf("%c%zu%c:",
            i == nfa.start ? '>' : ' ',
            i,
            i == nfa.terminal ? '*' : ' ');
        for (const char *a = alphabet; *a; ++a) {
            const char c = *a;
            const size_t jump = s->jumps[(unsigned char) c];
            if (jump != NFA_INVALID_INDEX) {
                printf("  %c => %zu", c, jump);
            }
        }
        for (size_t j = 0; j < s->eps_jumps.size; ++j) {
            printf("  # => %zu", s->eps_jumps.data[j]);
        }
        printf("\n");
    }

    DFA dfa = nfa2dfa(nfa, alphabet, alphabet + strlen(alphabet));
    puts("Built DFA:");
    for (size_t i = 0; i < dfa.states.size; ++i) {
        DFA_State *s = &dfa.states.data[i];
        printf("%c%zu%c:",
            i == dfa.start ? '>' : ' ',
            i,
            s->terminal ? '*' : ' ');
        for (const char *a = alphabet; *a; ++a) {
            const char c = *a;
            printf("  %c => %zu", c, s->jumps[(unsigned char) c]);
        }
        printf("\n");
    }

    char reply[2];
    do {
        fprintf(stderr, "Complement? [y/n] ");
    } while (scanf("%1s", reply) != 1 || !(reply[0] == 'y' || reply[0] == 'n'));

    if (reply[0] == 'y') {
        dfa_complement(&dfa);
        puts("Complemented the DFA.");
    }

    puts("Built regex:");
    LSString s = dfa2re(dfa, alphabet, alphabet + strlen(alphabet));
    printf("%.*s\n", (int) s.size, s.data);
    LS_VECTOR_FREE(s);
}
