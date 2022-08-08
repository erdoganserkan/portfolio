#include <cmath>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;

class mqueue {
    vector<int> buf;
public:
    void enqueue(int x) {
        buf.push_back(x);
    }
    void dequeue(void) {
        buf.erase(buf.begin());
    }
    void print(void) {
        cout << buf[0] << '\n';
    }
};

enum class query_t {
    ENQUEUE = 1,
    DEQUEUE,
    PRINT,

    QUEUE_QUERY_CNT
};

int main() {

    mqueue mq;
    int q;
    cin >> q;
    //cout << "q_cnt(" << q << ")\n";

    for(int ix=0 ; ix<q ; ix++) {
        int qry;
        int num;
        cin >> qry;
        //cout << "qry(" << qry << ")\n";
        if(query_t::ENQUEUE == static_cast<query_t>(qry)) {
            cin >> num;
            //cout << "num(" << num << ")\n";
        }

        switch(static_cast<query_t>(qry)) {
            case query_t::ENQUEUE:
                mq.enqueue(num);
                break;
            case query_t::DEQUEUE:
                mq.dequeue();
                break;
            case query_t::PRINT:
                mq.print();
                break;
            default:
                cout << "ERR\n";
                break;
        }
    }
    /* Enter your code here. Read input from STDIN. Print output to STDOUT */
    return 0;
}
