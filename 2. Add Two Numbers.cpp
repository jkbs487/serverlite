//https://leetcode-cn.com/problems/add-two-numbers/

/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     ListNode *next;
 *     ListNode(int x) : val(x), next(NULL) {}
 * };
 */
class Solution {
public:
    ListNode* addTwoNumbers(ListNode* l1, ListNode* l2) {
        ListNode* res = new ListNode(0);
        ListNode* temp = res;
        int carry = 0, sum = 0;
        if(l1 == NULL) return l2;
        if(l2 == NULL) return l1;
        while(l1 != NULL || l2 != NULL){
            int x = (l1 != NULL) ? l1->val : 0;
            int y = (l2 != NULL) ? l2->val : 0;
            sum = x + y + carry;
            if(sum >= 10 && temp != NULL) {
                temp->next = new ListNode(sum % 10);
                carry = 1;
            }
            if(sum < 10 && temp != NULL){
                temp->next = new ListNode(sum);
                carry = 0;
            }
            if(l1 != NULL) l1 = l1->next;
            if(l2 != NULL) l2 = l2->next;
            temp = temp->next;
        }
        if(carry == 1) temp->next = new ListNode(1);
        return res->next;
    }
};
