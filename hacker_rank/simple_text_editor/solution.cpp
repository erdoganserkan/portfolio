#include <cmath>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <memory>

#define ERR_COUT  {cout <<"ERROR @" << __FILE__ << ":" << __LINE__<<'\n';}

using namespace std;

enum class eop_t {
    OP_APPEND = 1,
    OP_DELETE = 2,
    OP_PRINT = 3,
    OP_UNDO = 4
};

struct sop_t {
    eop_t opt;
    string str;
    union {
        int ch_cnt;
        int ch_indx;
    };
};

string log_op(sop_t sop) {
    const string ops[]{"NA", "APPEND", "DELETE", "PRINT", "UNDO"};
    stringstream ret{};
    ret << "op(" << ops[static_cast<int>(sop.opt)] << ")";
    switch (sop.opt) {
        case eop_t::OP_APPEND:  // We will undo append with delete last ch_cnt characters //
            ret << ", append-str(" << sop.str << ")\n";
            break;
        case eop_t::OP_DELETE:  // Undo of delete: append deleted string //
            ret << ", del-ch-cnt(" << sop.ch_cnt << ")\n";
            break;
        case eop_t::OP_PRINT:
            ret << ", print-ch-indx(" << sop.ch_indx << ")\n";
            break;
        case eop_t::OP_UNDO:
            break;
        default:
            ERR_COUT;
            break;
    }
    return ret.str();
}

class mstack {
    vector <sop_t> opv;
public:
    void push(const sop_t& newop) {
        //cout << "add-stack: " << log_op(newop);
        opv.push_back(newop);
    }
    sop_t pop() {
        sop_t temp = opv[opv.size()-1];
        //cout << "pop-stack: " << log_op(temp);
        opv.erase(opv.end());
        return temp;
    }
};

class textedit {
    mstack te_stk;
    string te_str;
public:
    void process(sop_t sop);
    void input(void);
};

void textedit::process(sop_t sop) {
    switch(sop.opt) {
        case eop_t::OP_DELETE: {
            // Push delete operation details into stack for further undo operation //
            sop.str = te_str.substr(te_str.size()-sop.ch_cnt, sop.ch_cnt);
            te_stk.push(sop);
            // Do real character deletion from string //
            te_str.erase(te_str.size()-sop.ch_cnt, sop.ch_cnt);
            //cout << "last-string(" << te_str << ")\n";
        }
        break;
        case eop_t::OP_APPEND: {
            // STACK-JOB: Push operation details into stack //
            sop.ch_cnt = sop.str.size();
            te_stk.push(sop);
            // REAL-JOB: do string append //
            te_str.append(sop.str);
            //cout << "last-string(" << te_str << ")\n";
        } break;
        case eop_t::OP_PRINT:
            cout << te_str[sop.ch_indx-1] << '\n';
            break;
        case eop_t::OP_UNDO: {
            const sop_t msop = te_stk.pop();
            //cout << "from-stack: " << log_op(msop);
            switch (msop.opt) {
                case eop_t::OP_APPEND:  // We will undo append with delete last ch_cnt characters //
                    te_str.erase(te_str.size() - msop.ch_cnt, msop.ch_cnt);
                    break;
                case eop_t::OP_DELETE:  // Undo of delete: append deleted string //
                    te_str.append(msop.str);
                    break;
                case eop_t::OP_PRINT:
                    ERR_COUT;
                    break;
                case eop_t::OP_UNDO:
                    ERR_COUT;
                    break;
                default:
                    ERR_COUT;
                    break;
            }
        } break;
        default:
            ERR_COUT;
            break;
    }
}

void textedit::input(void) {
    int Q;
    cin >> Q;
    //cout << "input-total-cnt(" << Q << ")\n";
    // Read inputs from STDIN //
    for(int ix=0; ix<Q; ix++) {
        int iopt;
        cin >> iopt;
        sop_t csop;
        csop.opt = static_cast<eop_t>(iopt);

        switch(csop.opt) {
            case eop_t::OP_APPEND:
                cin >> csop.str;    // read to be appended string
                break;
            case eop_t::OP_DELETE:
                cin >> csop.ch_cnt; // read how many characters will be removed
                break;
            case eop_t::OP_PRINT:   // read the index of characters t be printed out
                cin >> csop.ch_indx;
                break;
            case eop_t::OP_UNDO:
                break;
            default:
                ERR_COUT;
                break;
        }
        //cout << "input: "<< log_op(csop);
        this->process(csop);
    }
    // Call process() after each of read-analysis input cycle completed //
}

int main() {
    textedit mte;

    mte.input();
    /* Enter your code here. Read input from STDIN. Print output to STDOUT */
    return 0;
}
