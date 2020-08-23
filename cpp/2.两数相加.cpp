/*
 * @lc app=leetcode.cn id=2 lang=cpp
 *
 * [2] 两数相加
 */

// @lc code=start
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
        ListNode* head = new ListNode();
        ListNode* cur = head;
        int carry = 0, sum;
        while(l1 != nullptr || l2 != nullptr) {
            int x = l1 ? l1->val : 0;
            int y = l2 ? l2->val : 0;
            sum = x + y + carry;
            carry = sum / 10;
            cur->next = new ListNode(sum%10);
            cur = cur->next;
            if(l1) l1 = l1->next;
            if(l2) l2 = l2->next;
        }
        if(carry) cur->next = new ListNode(1);
        return head->next;
    }
};
// @lc code=end

