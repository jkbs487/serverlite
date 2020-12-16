// Source : https://leetcode-cn.com/problems/path-sum-ii/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given a binary tree and a sum, find all root-to-leaf paths where each path's sum equals the given 
 * sum.
 * 
 * Note: A leaf is a node with no children.
 * 
 * Example:
 * 
 * Given the below binary tree and sum = 22,
 * 
 *       5
 *      / \
 *     4   8
 *    /   / \
 *   11  13  4
 *  /  \    / \
 * 7    2  5   1
 * 
 * Return:
 * 
 * [
 *    [5,4,11,2],
 *    [5,8,4,5]
 * ]
 * 
 ******************************************************************************************************/

class Solution {
public:
    vector<vector<int>> ans;
    vector<int> temp;
    vector<vector<int>> pathSum(TreeNode* root, int sum) {
        if(root == nullptr) return ans;
        temp.push_back(root->val);
        dfs(root, sum, root->val);
        return ans;
    }

    void dfs(TreeNode* root, int sum, int val) {
        if(root->left == nullptr && root->right == nullptr && val == sum) {
            ans.push_back(temp);
            return;
        } 
        if(root->left == nullptr && root->right == nullptr && val != sum) return;
        if(root->left != nullptr) {
            temp.push_back(root->left->val);
            dfs(root->left, sum, val+root->left->val);
            temp.pop_back();
        }
        if(root->right != nullptr) {
            temp.push_back(root->right->val);
            dfs(root->right, sum, val+root->right->val);
            temp.pop_back();
        }
        return;
    }
};
