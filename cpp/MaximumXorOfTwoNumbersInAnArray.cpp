// Source : https://leetcode-cn.com/problems/maximum-xor-of-two-numbers-in-an-array/
// Author : jkbs487
// Date   : 2021-05-24

/***************************************************************************************************** 
 *
 * Given an integer array nums, return the maximum result of nums[i] XOR nums[j], where 0 <= i &le; j 
 * < n.
 * 
 * Follow up: Could you do this in O(n) runtime?
 * 
 * Example 1:
 * 
 * Input: nums = [3,10,5,25,2,8]
 * Output: 28
 * Explanation: The maximum result is 5 XOR 25 = 28.
 * 
 * Example 2:
 * 
 * Input: nums = [0]
 * Output: 0
 * 
 * Example 3:
 * 
 * Input: nums = [2,4]
 * Output: 6
 * 
 * Example 4:
 * 
 * Input: nums = [8,10,2]
 * Output: 10
 * 
 * Example 5:
 * 
 * Input: nums = [14,70,53,83,49,91,36,80,92,51,66,70]
 * Output: 127
 * 
 * Constraints:
 * 
 * 	1 <= nums.length <= 2 * 104
 * 	0 <= nums[i] <= 231 - 1
 ******************************************************************************************************/

//Trie, left -> 0; right -> 1
//greedy algorithm
class Solution {
public:
    typedef struct Trie{
        Trie* left;
        Trie* right;
    };
    
    Trie* initTrie(vector<int> nums) {
        Trie* head = new Trie();
        for (int num : nums) {
            Trie* cur = head;
            int i = 30;
            while (i >= 0) {
                int index = num >> i & 1;
                if(index == 0) {
                    // 0 is left
                    if (!cur->left) cur->left = new Trie();
                    cur = cur->left;
                }
                else if (index == 1) {
                    // 1 is right
                    if (!cur->right) cur->right = new Trie();
                    cur = cur->right;
                }
                i--;
            }
        }
        return head;
    }

    int maxXOR(Trie* head, int num) {
        Trie* cur = head;
        int res = 0;
        int i = 30;
        while (cur && i >= 0) {
            int index = num >> i & 1;
            if (index == 0) {
                // greedy, choose the opposite number
                if (cur->right) {
                    cur = cur->right;
                    res += 1 << i;
                }
                else cur = cur->left;
            }
            else if (index == 1) {
                // greedy, choose the opposite number
                if (cur->left) {
                    cur = cur->left;
                    res += 1 << i;
                }
                else cur = cur->right;
            }
            i--;
        }
        return res;
    }

    int findMaximumXOR(vector<int>& nums) {
        int res = 0;
        Trie* head = initTrie(nums);
        for (int num : nums) {
            res = max(res, maxXOR(head, num));
        }
        return res;
    }
};
