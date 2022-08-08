#include <iostream>

using namespace std;

struct SinglyLinkedListNode {
     int data;
     SinglyLinkedListNode* next;
};

// Complete the mergeLists function below.
using SN = SinglyLinkedListNode ;

void printll(SN *h1) {
    cout << "ll: ";
    for(SN* temp = h1; temp ; temp=temp->next)
        cout  << temp->data<< ' ';
    cout << '\n';
}

/*
 * For your reference:
 *
 * SinglyLinkedListNode {
 *     int data;
 *     SinglyLinkedListNode* next;
 * };
 *
 */
SinglyLinkedListNode* mergeLists(SinglyLinkedListNode* head1, SinglyLinkedListNode* head2) {
    SN* h2 = head2;
    while(h2) {
        SN* h2next = h2->next;
        int d2 = h2->data;
        bool inserted = false;
        SN* h1=head1;
        for(; h1 && (h1->next); h1=h1->next) {
            int d11 = h1->data;
            int d12 = h1->next->data;
            if(d2 < d11) {
                inserted = true;
                h2->next = h1;
                head1 = h2;
                printll(head1);
                break;
            }
            else if((d2 >= d11) && (d2 <= d12)) {
                inserted = true;
                h2->next = h1->next;
                h1->next = h2;
                break;
            }
        }
        if(!inserted) {
            h2->next = NULL;
            h1->next = h2;  // Add h2 to the end oh head1 list //
        }
        h2 = h2next;
    }
    return head1;
}

int main() {

}

