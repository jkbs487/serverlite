/*
 * @lc app=leetcode.cn id=19 lang=cpp
 *
 * [19] 删除链表的倒数第N个节点
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
    ListNode* removeNthFromEnd(ListNode* head, int n) {
        ListNode *first = head, *mid = head, *last = head;
        while(n-- > 0) last = last->next;
        while(last != nullptr) {
            last = last->next;
            if(mid != head) first = first->next;
            mid = mid->next;
        }
        if(mid != head) first->next = mid->next;
        else head = head->next;
        mid = nullptr;
        return head;
    }
};
// @lc code=end

