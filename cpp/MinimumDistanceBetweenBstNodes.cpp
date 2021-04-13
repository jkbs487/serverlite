// Source : https://leetcode-cn.com/problems/minimum-distance-between-bst-nodes/
// Author : jkbs487
// Date   : 2021-04-13

/***************************************************************************************************** 
 *
 * Given the root of a Binary Search Tree (BST), return the minimum difference between the values of 
 * any two different nodes in the tree.
 * 
 * Note: This question is the same as 530: 
 * https://leetcode.com/problems/minimum-absolute-difference-in-bst/
 * 
 * Example 1:
 * 
 * Input: root = [4,2,6,1,3]
 * Output: 1
 * 
 * Example 2:
 * 
 * Input: root = [1,0,48,null,null,12,49]
 * Output: 1
 * 
 * Constraints:
 * 
 * 	The number of nodes in the tree is in the range [2, 100].
 * 	0 <= Node.val <= 105
 ******************************************************************************************************/

/**
 * Definition for a binary tree node.
 * struct TreeNode {
 *     int val;
 *     TreeNode *left;
 *     TreeNode *right;
 *     TreeNode() : val(0), left(nullptr), right(nullptr) {}
 *     TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
 *     TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
 * };
 */
class Solution {
public:
    int ans = INT_MAX;
    int temp = INT_MAX;
    int minDiffInBST(TreeNode* root) {
        inorder(root);
        return ans;
    }

    void inorder(TreeNode* root) {
        if(!root) return;
        inorder(root->left);
        if(temp < INT_MAX){
            ans = min(ans, abs(temp-root->val));
        }
        temp = root->val;
        inorder(root->right);
    }
};
