// Source : https://leetcode-cn.com/problems/remove-nth-node-from-end-of-list/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given the head of a linked list, remove the nth node from the end of the list and return its head.
 * 
 * Follow up: Could you do this in one pass?
 * 
 * Example 1:
 * 
 * Input: head = [1,2,3,4,5], n = 2
 * Output: [1,2,3,5]
 * 
 * Example 2:
 * 
 * Input: head = [1], n = 1
 * Output: []
 * 
 * Example 3:
 * 
 * Input: head = [1,2], n = 1
 * Output: [1]
 * 
 * Constraints:
 * 
 * 	The number of nodes in the list is sz.
 * 	1 <= sz <= 30
 * 	0 <= Node.val <= 100
 * 	1 <= n <= sz
 ******************************************************************************************************/

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
