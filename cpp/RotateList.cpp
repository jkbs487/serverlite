// Source : https://leetcode-cn.com/problems/rotate-list/
// Author : jkbs487
// Date   : 2021-03-27

/***************************************************************************************************** 
 *
 * Given the head of a linked list, rotate the list to the right by k places.
 * 
 * Example 1:
 * 
 * Input: head = [1,2,3,4,5], k = 2
 * Output: [4,5,1,2,3]
 * 
 * Example 2:
 * 
 * Input: head = [0,1,2], k = 4
 * Output: [2,0,1]
 * 
 * Constraints:
 * 
 * 	The number of nodes in the list is in the range [0, 500].
 * 	-100 <= Node.val <= 100
 * 	0 <= k <= 2 * 109
 ******************************************************************************************************/

/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     ListNode *next;
 *     ListNode(int x) : val(x), next(NULL) {}
 * };
 */

//use pre node do three things,
//1. cal list length
//2. points to tail node of this list
//3. Closed list
class Solution {
public:
    ListNode* rotateRight(ListNode* head, int k) {
        if(head == nullptr) return nullptr;
        ListNode* pre = head;
        int len = 0;
        while(pre != nullptr) {
            len++;
            if(pre->next == nullptr) {
                pre->next = head;
                break;
            }
            pre = pre->next;
        }
        int skipDis = len - (k % len);
        while(skipDis-- > 0) {
            pre = pre->next;
            head = head->next;
        }
        pre->next = nullptr;
        return head;
    }
};
