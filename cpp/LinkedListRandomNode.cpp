// Source : https://leetcode-cn.com/problems/linked-list-random-node/
// Author : jkbs487
// Date   : 2021-04-24

/***************************************************************************************************** 
 *
 * Given a singly linked list, return a random node's value from the linked list. Each node must have 
 * the same probability of being chosen.
 * 
 * Example 1:
 * 
 * Input
 * ["Solution", "getRandom", "getRandom", "getRandom", "getRandom", "getRandom"]
 * [[[1, 2, 3]], [], [], [], [], []]
 * Output
 * [null, 1, 3, 2, 2, 3]
 * 
 * Explanation
 * Solution solution = new Solution([1, 2, 3]);
 * solution.getRandom(); // return 1
 * solution.getRandom(); // return 3
 * solution.getRandom(); // return 2
 * solution.getRandom(); // return 2
 * solution.getRandom(); // return 3
 * // getRandom() should return either 1, 2, or 3 randomly. Each element should have equal probability 
 * of returning.
 * 
 * Constraints:
 * 
 * 	The number of nodes in the linked list will be in the range [1, 104].
 * 	-104 <= Node.val <= 104
 * 	At most 104 calls will be made to getRandom.
 * 
 * Follow up:
 * 
 * 	What if the linked list is extremely large and its length is unknown to you?
 * 	Could you solve this efficiently without using extra space?
 ******************************************************************************************************/

/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     ListNode *next;
 *     ListNode() : val(0), next(nullptr) {}
 *     ListNode(int x) : val(x), next(nullptr) {}
 *     ListNode(int x, ListNode *next) : val(x), next(next) {}
 * };
 */
//Reservoir Sampling
class Solution {
    ListNode *head;
public:
    /** @param head The linked list's head.
        Note that the head is guaranteed to be not null, so it contains at least one node. */
    Solution(ListNode* head) {
        this->head = head;
        srand(time(0));
    }
    
    /** Returns a random node's value. */
    int getRandom() {
        int count = 1;
        int value = 0;
        ListNode *cur = head;
        while (cur) {
            if(rand()%count == 0)
                value = cur->val;
            count++;
            cur = cur->next;
        }
        return value;
    }
};

/**
 * Your Solution object will be instantiated and called as such:
 * Solution* obj = new Solution(head);
 * int param_1 = obj->getRandom();
 */
