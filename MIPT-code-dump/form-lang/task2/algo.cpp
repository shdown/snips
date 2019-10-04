#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <stdlib.h>
#include <assert.h>

struct Rule
{
    char lhs;
    std::string rhs;
};

struct Grammar
{
    std::vector<Rule> rules;
    char start;
};

static const char FAKE_NONTERMINAL = '@';

static inline bool terminal(char c)
{
    return 'a' <= c && c <= 'z';
}

static inline bool nonterminal(char c)
{
    return ('A' <= c && c <= 'Z') || c == FAKE_NONTERMINAL;
}

struct PreprocessedGrammar : public std::vector<Rule>
{
    PreprocessedGrammar(const Grammar &g)
        : std::vector<Rule>{Rule{
            FAKE_NONTERMINAL,
            std::string(1, g.start)
        }}
    {
        insert(end(), g.rules.begin(), g.rules.end());
    }
};

struct Situation
{
    size_t rule_index;
    size_t rhs_dot_index;
    size_t input_index;

    Situation advance() const
    {
        return {rule_index, rhs_dot_index + 1, input_index};
    }

    bool operator <(Situation s) const
    {
        if (rule_index != s.rule_index) {
            return rule_index < s.rule_index;
        }
        if (rhs_dot_index != s.rhs_dot_index) {
            return rhs_dot_index < s.rhs_dot_index;
        }
        return input_index < s.input_index;
    }
};

class Algo
{
    PreprocessedGrammar rules_;

    char symbol_after_dot_(Situation s) const
    {
        return rules_[s.rule_index].rhs.c_str()[s.rhs_dot_index];
    }

    char producing_nonterminal_(Situation s) const
    {
        return rules_[s.rule_index].lhs;
    }

public:
    explicit Algo(const PreprocessedGrammar &rules)
        : rules_(rules) {}

    void scan(
        Situation s,
        char letter,
        std::set<Situation> &output) const
    {
        if (symbol_after_dot_(s) == letter) {
            output.insert(s.advance());
        }
    }

    void complete(
        Situation s,
        std::vector<std::set<Situation>> &d,
        std::vector<Situation> &output) const
    {
        const char nt = producing_nonterminal_(s);
        for (Situation t : d[s.input_index]) {
            if (symbol_after_dot_(t) == nt) {
                output.push_back(t.advance());
            }
        }
    }

    void predict(
        Situation s,
        size_t index,
        std::vector<Situation> &output) const
    {
        const char c = symbol_after_dot_(s);
        for (size_t i = 0; i < rules_.size(); ++i) {
            if (rules_[i].lhs == c) {
                output.push_back(Situation{i, 0, index});
            }
        }
    }

    bool accepts(const std::string &word) const
    {
        std::vector<std::set<Situation>> d(word.size() + 1);
        d[0].insert(Situation{0, 0, 0});

        for (size_t i = 0; i <= word.size(); ++i) {
            std::set<Situation> &cur_set = d[i];

            std::vector<Situation> unprocessed(cur_set.begin(), cur_set.end());
            cur_set.clear();
            while (!unprocessed.empty()) {
                Situation s = unprocessed.back();
                unprocessed.pop_back();

                if (cur_set.count(s) != 0) {
                    continue;
                }
                cur_set.insert(s);

                const char c = symbol_after_dot_(s);
                if (c == '\0') {
                    complete(s, d, unprocessed);
                } else if (terminal(c)) {
                    if (i != word.size()) {
                        scan(s, word[i], d[i + 1]);
                    }
                } else if (nonterminal(c)) {
                    predict(s, i, unprocessed);
                } else {
                    assert(!"symbol is not end-of-string, terminal or non-terminal");
                }
            }
        }

        return d[word.size()].count(Situation{0, 1, 0}) != 0;
    }
};

int
main()
{
    char start_nonterm;
    std::string word;
    std::cin >> start_nonterm >> word;
    if (word == "-") {
        word = "";
    }

    std::vector<Rule> rules;
    char lhs;
    std::string rhs;
    while (std::cin >> lhs >> rhs) {
        if (rhs == "-") {
            rhs = "";
        }
        rules.emplace_back(Rule{lhs, rhs});
    }

    const bool matches =
        Algo(
            PreprocessedGrammar{
                Grammar{
                    rules,
                    start_nonterm,
                }
            }
        ).accepts(word);

    std::cout << (matches ? "YES" : "NO") << std::endl;
}
